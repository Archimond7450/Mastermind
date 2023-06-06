#include <array>
#include <cstdlib>
#include <icecream.hpp>
#include <vector>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#define INFO(msg)               \
    {                           \
        const char *info = msg; \
        IC(info);               \
    }
#define ERROR(msg)               \
    {                            \
        const char *error = msg; \
        IC(error);               \
    }

struct PinState
{
    std::array<unsigned int, 5> pixelColorIndexes = {UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX};
};

struct PinGuessResult
{
    int correctColorPosition = 0; // Black
    int correctPositionOnly = 0; // White

    PinGuessResult() = delete;
    PinGuessResult(PinState guess, PinState actual)
    {
        for (int i = 0; i < 5; ++i)
        {
            if (guess.pixelColorIndexes[i] == actual.pixelColorIndexes[i])
            {
                ++correctColorPosition;
                continue;
            }
            for (int j = 0; j < 5; ++j)
            {
                if (i != j && guess.pixelColorIndexes[i] == actual.pixelColorIndexes[j])
                {
                    ++correctPositionOnly;
                    break;
                }
            }
        }
    };
};

struct PinGuess
{
    PinState state;
    PinGuessResult result;

    PinGuess() = delete;
    PinGuess(PinState guess, PinState actual) : state(guess), result(guess, actual)
    {

    };
};

struct Board
{
    std::vector<PinGuess> guesses;
};

class App
{
    public:
        App() = default;
        App(const App &other) = delete;
        App(App &&app) = default;
        ~App()
        {
            if (this->bigFont)
            {
                INFO("Unloading the big font");
                XFreeFont(this->dpy, this->bigFont);
                this->bigFont = nullptr;
            }

            if (this->gc)
            {
                INFO("Freeing Graphics Context");
                XFreeGC(this->dpy, this->gc);
                this->gc = nullptr;
            }

            if (this->window)
            {
                INFO("Unmapping and destroying window");
                XUnmapWindow(this->dpy, this->window);
                XDestroyWindow(this->dpy, this->window);
                this->window = 0L;
            }

            if (this->dpy)
            {
                INFO("Closing display");
                XCloseDisplay(this->dpy);
                this->dpy = nullptr;
            }
        }

        bool Initialize()
        {
            if (once)
            {
                INFO("Initialize");
                once = false;

                INFO("Opening display");
                this->dpy = XOpenDisplay(nullptr);
                if (this->dpy == nullptr)
                {
                    ERROR("Cannot open display connection!");
                    return false;
                }

                INFO("Gathering required data to create window");
                this->scr = DefaultScreen(dpy);

                this->colorBlack = BlackPixel(dpy, scr);
                this->colorWhite = WhitePixel(dpy, scr);

                this->rootWindow = RootWindow(dpy, scr);

                this->depth = DefaultDepth(this->dpy, this->scr);
                this->visual = DefaultVisual(this->dpy, this->scr);

                XSetWindowAttributes xwa =
                {
                    .background_pixel = this->colorWhite,
                    .border_pixel = this->colorBlack,
                    .event_mask = StructureNotifyMask | Button1MotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | ExposureMask
                };
                auto xwaMask = CWBackPixel | CWEventMask | CWBorderPixel;

                INFO("Creating window");
                this->window = XCreateWindow(
                    this->dpy,
                    this->rootWindow,
                    this->x,
                    this->y,
                    this->width,
                    this->height,
                    this->borderWidth,
                    this->depth,
                    InputOutput,
                    this->visual,
                    xwaMask,
                    &xwa);
                if (!this->window)
                {
                    ERROR("Cannot create window!");
                    return false;
                }

                INFO("Creating colors");
                this->cmap = DefaultColormap(this->dpy, this->scr);

                if (
                    !this->CreateColor(&this->colorRed, this->red) ||
                    !this->CreateColor(&this->colorPink, this->pink) ||
                    !this->CreateColor(&this->colorDarkGoldenrod, this->darkGoldenrod) ||
                    !this->CreateColor(&this->colorGray, this->gray) ||
                    !this->CreateColor(&this->colorMediumSeaGreen, this->mediumSeaGreen) ||
                    !this->CreateColor(&this->colorYellow, this->yellow)
                )
                {
                    return false;
                }

                this->colors[0] = this->colorWhite;
                this->colors[1] = this->colorBlack;
                this->colors[2] = this->colorRed.pixel;
                this->colors[3] = this->colorPink.pixel;
                this->colors[4] = this->colorDarkGoldenrod.pixel;
                this->colors[5] = this->colorGray.pixel;
                this->colors[6] = this->colorMediumSeaGreen.pixel;
                this->colors[7] = this->colorYellow.pixel;

                XGCValues xgcv =
                {
                    .foreground = this->colors[1],
                    .background = this->colors[0],
                    .line_width = 5,
                    .line_style = LineSolid,
                    .cap_style = CapButt,
                    .join_style = JoinRound,
                    .fill_style = FillSolid
                };
                auto xgcvMask = GCForeground | GCBackground | GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle | GCFillStyle;

                INFO("Creating Graphics Context");
                this->gc = XCreateGC(this->dpy, this->rootWindow, xgcvMask, &xgcv);
                if (this->gc == nullptr)
                {
                    ERROR("Cannot create Graphics Context");
                    return false;
                }

                INFO("Loading big font (12x24)");
                this->bigFont = XLoadQueryFont(this->dpy, "12x24");
                if (this->bigFont == nullptr)
                {
                    ERROR("Cannot load big font");
                    return false;
                }
            }

            return true;
        }

        void Run()
        {
            XSizeHints xsh =
            {
                .flags = PMinSize | PMaxSize,
                .min_width = static_cast<int>(this->width),
                .min_height = static_cast<int>(this->height),
                .max_width = static_cast<int>(this->width),
                .max_height = static_cast<int>(this->height)
            };
            XSetSizeHints(this->dpy, this->window, &xsh, XA_WM_NORMAL_HINTS);

            XStoreName(this->dpy, this->window, "Logik");
            XMapWindow(this->dpy, this->window);
            this->WMDeleteWindow = XInternAtom(this->dpy, "WM_DELETE_WINDOW", False);
            XSetWMProtocols(this->dpy, this->window, &this->WMDeleteWindow, True);

            bool done = false;
            while (!done)
            {
                XEvent e;
                while (XPending(this->dpy))
                {
                    XNextEvent(dpy, &e);
                    IC(e.type);
                    switch (e.type)
                    {
                        case MapNotify:
                        {
                            INFO("MapNotify");

                            this->Render();

                            // Send render request to server
                            XFlush(this->dpy);
                            break;
                        }
                        case KeyPress:
                        {
                            INFO("KeyPress");
                            XKeyPressedEvent event = e.xkey;
                            auto keySym = XkbKeycodeToKeysym(this->dpy, event.keycode, 0, 0);
                            IC(event.keycode, keySym);
                            this->KeyPressHandler(keySym, done);
                            break;
                        }
                        case ButtonPress:
                        {
                            INFO("ButtonPress");
                            XButtonPressedEvent event = e.xbutton;
                            IC(event.button, event.x, event.y, event.x_root, event.y_root);
                            if (event.button == Button1)
                            {
                                this->Update(event.x, event.y);
                                XClearWindow(this->dpy, this->window);
                                this->Render();
                                XFlush(this->dpy);
                            }
                            break;
                        }
                        // Re-render
                        case Expose:
                        case GraphicsExpose:
                        {
                            INFO("Expose/GraphicsExpose");
                            IC(e.type);

                            this->Render();
                            XFlush(this->dpy);
                            
                            break;
                        }
                        case MotionNotify:
                        {
                            INFO("MotionNotify");
                            break;
                        }
                        case ClientMessage:
                        {
                            auto event = e.xclient;
                            auto receivedAtom = static_cast<Atom>(event.data.l[0]);
                            if (receivedAtom == this->WMDeleteWindow)
                            {
                                // Window was closed
                                INFO("Window closed. Exiting.");
                                done = true;
                            }
                            break;
                        }
                        default:
                        {
                            INFO("other event");
                            break;
                        }
                    }
                }
            }
        }

    protected:
        static PinState GetRandomPins()
        {
            srand(time(nullptr));
            return
            {
                .pixelColorIndexes =
                {
                    static_cast<unsigned int>(rand()) % 8,
                    static_cast<unsigned int>(rand()) % 8,
                    static_cast<unsigned int>(rand()) % 8,
                    static_cast<unsigned int>(rand()) % 8,
                    static_cast<unsigned int>(rand()) % 8
                }
            };
        }

        bool CreateColor(XftColor *color, const char *name)
        {
            if (!XftColorAllocName(this->dpy, this->visual, this->cmap, name, color))
            {
                ERROR("Cannot allocate color");
                IC(name);
                return false;
            }

            color->pixel |= 0xff << 24; // Make fully opaque
            return true;
        }

        void Render()
        {
            XSetFont(this->dpy, this->gc, this->bigFont->fid);
            XSetLineAttributes(this->dpy, this->gc, 10, LineSolid, CapButt, JoinRound);
            
            // Draw the available colors
            for (int i = 0; i < 8;++i)
            {
                this->DrawCircle(i * 70 + 25, 920, 55, this->colors[i]);
            }

            int y = 820;
            for (auto oneGuess = this->board.guesses.begin(); oneGuess != board.guesses.end() ; ++oneGuess)
            {
                int x = 25;

                // Draw guess
                for (int i = 0; i < 5; ++i)
                {
                    this->DrawCircle(x, y, 55, this->colors[oneGuess->state.pixelColorIndexes[i]]);
                    x += 70;
                }

                // Draw correct color positions (black)
                for (int i = 0; i < oneGuess->result.correctColorPosition; ++i)
                {
                    this->DrawCircle(x, y + 20, 30, this->colors[1]);
                    x += 45;
                }

                // Draw correct colors only (white)
                for (int i = 0; i < oneGuess->result.correctPositionOnly; ++i)
                {
                    this->DrawCircle(x, y + 20, 30, this->colors[0]);
                    x += 45;
                }

                y -= 80;
            }

            int x = 25;
            if (this->board.guesses.size() < 10)
            {
                // Draw the guess in progress
                for (auto it = this->guess.pixelColorIndexes.begin(); it != this->guess.pixelColorIndexes.end(); ++it)
                {
                    if (*it >= 8)
                    {
                        break;
                    }
                    this->DrawCircle(x, y, 55, this->colors[*it]);
                    x += 70;
                }
            }
        }

        void Update(int x, int y)
        {
            if (this->IsGameOver())
            {
                return;
            }

            if (y > 920)
            {
                if (x > 20)
                {
                    auto colorIndex = (x - 20) / 70;
                    
                    if (colorIndex >= 0 && colorIndex < 8)
                    {
                        for (auto it = this->guess.pixelColorIndexes.begin(); it != this->guess.pixelColorIndexes.end(); ++it)
                        {
                            if (*it >= 8)
                            {
                                *it = colorIndex;
                                break;
                            }
                        }
                    }
                    else if (this->guess.pixelColorIndexes[4] < 8)
                    {
                        this->MakeAGuess();
                    }
                }
                else if (x < 20)
                {
                    this->RevertOneGuessPin();
                }
            }
            
            //this->board.guesses.emplace_back(GetRandomPins(), this->actualPins);

            this->AddActual();
        }

        void KeyPressHandler(KeySym keySym, bool &done)
        {
            if (keySym == XK_Escape)
            {
                done = true;
            }
            else if (!this->IsGameOver())
            {
                if (keySym == XK_BackSpace)
                {
                    this->RevertOneGuessPin();
                }
                else if (keySym == XK_KP_Enter || keySym == XK_ISO_Enter)
                {
                    this->MakeAGuess();
                }

                this->AddActual();
                XClearWindow(this->dpy, this->window);
                this->Render();
                XFlush(this->dpy);
            }
        }

        // Is the game over - victorious or not (true) or is the game still in progress (false)
        bool IsGameOver() const
        {
            return (this->board.guesses.size() != 0 && (this->board.guesses.size() >= 10 || this->board.guesses[this->board.guesses.size() - 1].result.correctColorPosition == 5));
        }

        void RevertOneGuessPin()
        {
            if (this->guess.pixelColorIndexes[4] < 8)
            {
                this->guess.pixelColorIndexes[4] = UINT_MAX;
            }
            else
            {
                for (auto it = this->guess.pixelColorIndexes.begin(); it != this->guess.pixelColorIndexes.end(); ++it)
                {
                    if (*it >= 8)
                    {
                        if (it != this->guess.pixelColorIndexes.begin())
                        {
                            it--;
                            *it = UINT_MAX;
                        }
                        break;
                    }
                }
            }
        }

        void MakeAGuess()
        {
            this->board.guesses.emplace_back(this->guess, this->actualPins);
            this->guess.pixelColorIndexes.fill(UINT_MAX);
        }

        void AddActual()
        {
            if (this->board.guesses.size() == 10 && this->board.guesses[this->board.guesses.size() - 1].result.correctColorPosition < 5)
            {
                this->board.guesses.emplace_back(this->actualPins, this->actualPins);
            }
        }

        template <std::size_t N> void DrawString(int x, int y, const char (&string)[N])        {
            XDrawString(this->dpy, this->window, this->gc, x, y, string, N - 1);
        }

        void DrawCircle(int x, int y, unsigned int r, unsigned long pixelColor)
        {
            XSetForeground(this->dpy, this->gc, this->colorBlack);
            XDrawArc(this->dpy, this->window, this->gc, x, y, r, r, 0, 360 * 64);
            XSetForeground(this->dpy, this->gc, pixelColor);
            XFillArc(this->dpy, this->window, this->gc, x, y, r, r, 0, 360 * 64);
        }

    private:
        bool once = true; // Allow app to initialize only once
        Display *dpy = nullptr; // Display connection
        int scr = 0;
        unsigned long colorBlack = 0L;
        unsigned long colorWhite = 0L;
        Window rootWindow = 0UL;
        int x = 0, y = 0;
        unsigned int width = 600U, height = 1000U, borderWidth = 0U;
        int depth = 0;
        Visual *visual = nullptr;
        Window window = 0L;
        GC gc = nullptr; // Graphics context
        Colormap cmap;
        const char *red = "#FF0000";
        XftColor colorRed;
        const char *pink = "#FFC0CB";
        XftColor colorPink;
        const char *darkGoldenrod = "#B8860B";
        XftColor colorDarkGoldenrod;
        const char *gray = "#BEBEBE";
        XftColor colorGray;
        const char *mediumSeaGreen = "#3CB371";
        XftColor colorMediumSeaGreen;
        const char *yellow = "#FFFF00";
        XftColor colorYellow;
        XFontStruct *bigFont = nullptr;
        Atom WMDeleteWindow;
        PinState actualPins = GetRandomPins();
        Board board;
        PinState guess;
        std::array<unsigned long, 8> colors;
};

int main(int argc, char *argv[])
{
    App app;
    if (!app.Initialize())
    {
        return EXIT_FAILURE;
    }

    try
    {
        app.Run();
    }
    catch (std::exception &e)
    {
        INFO("Exception");
        IC(e);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;    
}