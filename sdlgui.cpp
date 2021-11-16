#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string>
#include <vector>

const int FRAMES_PER_SECOND = 20;
int screen_width;
int screen_height;

class Display
{
private:
public:
  SDL_Window *main_window;
  SDL_Renderer *main_renderer;

  Display(){};
  ~Display(){};

  SDL_Renderer *CreateRenderer(SDL_Window *w)
  {
    SDL_Renderer *renderer = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    return renderer;
  }

  void Init()
  {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
      fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
    }
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);
    screen_width = DM.w; //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    screen_height = DM.h;
    printf("Initialised with screen Width:%i Height%i\n", screen_width, screen_height);

    SDL_ShowCursor(SDL_DISABLE);

    main_window = SDL_CreateWindow(
        "Rob's Test Window",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        screen_width, screen_height,
        SDL_WINDOW_SHOWN);
    if (main_window == NULL)
    {
      fprintf(stderr, "could not create window: %s\n", SDL_GetError());
    }
    main_renderer = CreateRenderer(main_window);
  }
};

class Font
{
private:
public:
  TTF_Font *font;
  SDL_Rect dstrect;
  int c_width;
  int c_height;

  Font(const char *fontpath, int size) //size is pixels wide
  {
    TTF_Init();
    font = TTF_OpenFont(fontpath, size);
    if (font == NULL)
    {
      fprintf(stderr, "error: font not found\n");
      exit(EXIT_FAILURE);
    }
    TTF_SizeText(font, "B", &c_width, &c_height); //get a rough idea of font charachter size in pixels
    float scale = ((float)size / c_width);
    //printf("Scale=%f\n", scale);

    if (scale > 1.0 | scale < 0.9) //re-scale if reasonable difference to requested
    {
      font = TTF_OpenFont(fontpath, (int)size * scale);
      if (font == NULL)
      {
        fprintf(stderr, "error: font not found\n");
        exit(EXIT_FAILURE);
      }
      // TTF_SizeText(font, "B", &c_width, &c_height); //get a rough idea of font charachter size in pixels
      // scale = ((float)size / c_width);
      // printf("Scaled=%f\n", scale);
    }

    printf("Font Initialised\n");
  }

  SDL_Texture *CreateFontTexture(const char *text, SDL_Renderer *renderer, SDL_Color color)
  {
    SDL_Surface *fontsurface = TTF_RenderText_Blended(font, text, color);
    SDL_Texture *fonttexture = SDL_CreateTextureFromSurface(renderer, fontsurface);
    SDL_FreeSurface(fontsurface);
    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(fonttexture, NULL, NULL, &texW, &texH);
    dstrect = {0, 0, texW, texH};
    // printf("Creted Texture\n");
    return fonttexture;
  }

  void draw(int x, int y, SDL_Texture *fonttexture, SDL_Renderer *renderer)
  {
    dstrect.x = x;
    dstrect.y = y;
    SDL_RenderCopy(renderer, fonttexture, NULL, &dstrect);
  }
};

class FreqControl
{
private:
  SDL_Renderer *renderer;
  SDL_Rect rect;
  SDL_Rect temprect;
  SDL_Texture *digits_tex[11]; //0-9 plus '.'
  SDL_Rect digits_sizes[11];
  SDL_Rect digits[12]; // 12 digits, padded
  Font *font;
  long long int freq = 144750000;
  long long int maxfreq = 60000000000;
  long long int minfreq = 50000000;
  int num_digits = 12; //up to 999GHz
  int num_chars = 15;  //plus 3x dots

public:
  FreqControl(SDL_Renderer *_renderer, SDL_Rect _rect, Font *_font)
  {
    renderer = _renderer;
    rect = _rect;
    font = _font;
    //base_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, rect.w, rect.h); //create texture to work with.

    create_digits_tex();
  }

  void create_digits_tex()
  {

    char c;
    for (int i = 0; i < num_digits - 1; i++)
    {
      if (i > 9)
      {
        c = '.';
      }
      else
      {
        c = char(i + 48);
      }

      digits_tex[i] = font->CreateFontTexture(&c, renderer, (SDL_Color){0, 0, 255, 128});
      int texW = 0;
      int texH = 0;
      SDL_QueryTexture(digits_tex[i], NULL, NULL, &texW, &texH);
      digits_sizes[i] = (SDL_Rect){0, 0, texW, texH};
    }
  }

  void update_gui(SDL_Event e)
  {

    int x = e.tfinger.x * screen_width;
    int y = e.tfinger.y * screen_height;
    if (x<(rect.x + rect.w) & x> rect.x & y<(rect.y + rect.h) & y> rect.y)
    {
      //inside freq control element
      //loop through digit rects to find which has been pressed... >>> better way??
      for (int n = 0; n < num_digits; n++)
      {
        if (x<(digits[n].x + digits[n].w) & x> digits[n].x & y<(digits[n].y + digits[n].h) & y> digits[n].y)
        {
          //printf("Touched x=%i y=%i  n=%i    x:%i y:%i w:%i h:%i\n",x,y,n,digits[n].x,digits[n].y,digits[n].w,digits[n].h);
          if (y > (digits[n].y + (digits[n].h / 2)))
          {
            printf("Touched x=%i down\n", n);
            changefreq(num_digits - 1 - n, false);
          }
          else
          {
            printf("Touched x=%i up\n", n);
            changefreq(num_digits - 1 - n, true);
          }
        }
      }
    }
  }

  void changefreq(int digit, bool add)
  {
    long long int val = 1;
    for (int i = 0; i < digit; i++)
    {
      val *= 10;
    }
    //printf("val=%lli\n", val);
    if (add)
    {
      if (freq + val < maxfreq)
      {
        freq += val;
      }
    }
    else
    {
      if (freq - val > minfreq)
      {
        freq -= val;
      }
    }
  }

  void draw()
  {

    char freqstring[12];
    char freqstring_dots[15];
    sprintf(freqstring, "%012lld", freq); //get all number digits to array with 0 padding

    // printf("Freqstring=%s\n", freqstring);

    // add dots at thousand points
    int n = 0;
    for (int i = 0; i < num_chars + 1; i++)
    { //15 with extra decimal seperators
      for (int a = 0; a < 3; a++)
      {
        freqstring_dots[i] = freqstring[n];

        n++;
        i++;
      }

      freqstring_dots[i] = '.';
    }

    //draw box for freqcontrol area
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 25);
    SDL_RenderDrawRect(renderer, &rect); //draw border
    int pos;

    int final_width = 0;
    //printf("here\n");
    ////// Find with of all digits >>>>>>>>> better way?
    for (int n = 0; n < num_chars; n++)
    {

      if (freqstring_dots[n] == '.')
      {
        pos = 10;
      }
      else
      {
        pos = freqstring_dots[n] - 48;
      }
      final_width += digits_sizes[pos].w;
    }

    int offset = rect.x + ((rect.w - final_width) / 2);
    int d = 0;
    bool leadzero = true;
    for (int n = 0; n < num_chars; n++)
    {
      // printf("n=%i\n", n);
      int texW = 0;
      int texH = 0;
      if (freqstring_dots[n] == '.')
      {
        pos = 10;
      }
      else
      {
        pos = freqstring_dots[n] - 48;
      }
      temprect = (SDL_Rect){offset, rect.y + (rect.h - digits_sizes[pos].h) / 2, digits_sizes[pos].w, digits_sizes[pos].h};
      if (pos != 10)
      {
        //printf("d=%i\n", d);
        digits[d] = temprect;
        d++;
      }
      if (leadzero)
      {
        if (pos == 0 | pos == 10)
        {
          SDL_SetTextureAlphaMod(digits_tex[pos], 35);
          SDL_RenderCopy(renderer, digits_tex[pos], NULL, &temprect);
          SDL_SetTextureAlphaMod(digits_tex[pos], 255);
        }
        else
        {
          leadzero = false;
          SDL_SetTextureAlphaMod(digits_tex[pos], 255);
          SDL_RenderCopy(renderer, digits_tex[pos], NULL, &temprect);
        }
      }
      else
      {
        SDL_RenderCopy(renderer, digits_tex[pos], NULL, &temprect);
      }

      offset += digits_sizes[pos].w;
    }
  }
};

class Slider
{
private:
  float level = 0.0f;
  SDL_Renderer *renderer;
  SDL_Rect rect;
  SDL_Rect temprect;
  SDL_Texture *slider_tex;
  bool horizontal = false;
  int border_spacing = 2;

  void draw_border()
  {
    SDL_SetRenderTarget(renderer, slider_tex); //set draw to texture not main renderer
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    temprect = {0, 0, rect.w, rect.h}; //location and size of slider texture
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 25);
    SDL_RenderDrawRect(renderer, &temprect); //draw border
    SDL_SetRenderTarget(renderer, NULL);     ////set draw to main renderer
  }

public:
  Slider(SDL_Renderer *_renderer, SDL_Rect _rect, bool _horizontal, float initlevel) //pass in renderer pointer on creation and store it;
  {
    horizontal = _horizontal;
    renderer = _renderer;
    rect = _rect;
    level = initlevel;
    slider_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, rect.w, rect.h); //create texture to work with.
    update_normalised(level);

    printf("Init Slider Texture\n");
  };

  void update_gui(SDL_Event e)
  {
    int x = e.tfinger.x * screen_width;
    int y = e.tfinger.y * screen_height;
    if (x<(rect.x + rect.w) & x> rect.x & y<(rect.y + rect.h) & y> rect.y)
    {
      if (horizontal)
      {
        level = (float)(x - rect.x + border_spacing) / rect.w;
      }
      else
      {
        level = 1.0 - (float)(y - rect.y + border_spacing) / rect.y;
      }
      update_normalised(level);
      printf("Level %f\n", level);
    }
  }

  void update_normalised(float slider_level)
  {
    //printf("Slider level:%f\n",slider_level);
    draw_border();

    SDL_SetRenderTarget(renderer, slider_tex); //set draw to texture not main renderer
    if (horizontal)
    {
      SDL_SetRenderDrawColor(renderer, (slider_level * 255), 255 - (slider_level * 255), 0, 128);                                      //varing green > red slider colour vs level
      temprect = {border_spacing, border_spacing, (int)((slider_level)*rect.w) - (border_spacing * 2), rect.h - (border_spacing * 2)}; //location and size of slider texture
    }
    else
    {
      SDL_SetRenderDrawColor(renderer, slider_level * 255, 255 - (slider_level * 255), 0, 128);                                                                                                //varing green > red slider colour vs level
      temprect = {border_spacing, (int)((1.0 - slider_level) * rect.y) + border_spacing, rect.w - (border_spacing * 2), rect.y - (int)((1.0 - slider_level) * rect.y) - (border_spacing * 2)}; //location and size of slider texture
    }
    SDL_RenderFillRect(renderer, &temprect); //draw slider filled rectangle
    SDL_SetRenderTarget(renderer, NULL);     ////set draw to main renderer
  }

  void draw()
  {
    SDL_RenderCopy(renderer, slider_tex, NULL, &rect);
  }

  float get_val()
  {
    return level;
  }
};

class Layout
{
private:
public:
  SDL_Rect rect;

  Layout(int x, int y, int width, int height)
  {
    rect.w = width;
    rect.h = height;
    rect.x = x - (width / 2);
    rect.y = y - (height / 2);
    //printf("rect %i %i %i %i\n", rect.x, rect.y, rect.w, rect.h);
  }
};

class Button
{
private:
  // int x, y, w, h;
  SDL_Renderer *renderer;
  SDL_Rect rect;
  SDL_Rect temprect;
  SDL_Texture *button_tex;
  SDL_Texture *font_tex;
  SDL_Rect font_rect;
  Font *font;
  int texW, texH;

public:
  int selected = 0;

  // char *items[6] = {"Menu", "Mode", "Band", "Levels", "PTT", ""};

  // std::vector<SDL_Rect> rects;

  Button(SDL_Renderer *_renderer, SDL_Rect _rect, Font *_font)
  {
    renderer = _renderer;
    rect = _rect;
    font = _font;
    button_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, rect.w, rect.h); //create texture to work with.
                                                                                                                  // Button::selected=0;
    printf("initialised\n");
    update();
  }

  void update_gui(SDL_Event e)
  {
    int x = e.tfinger.x * screen_width;
    int y = e.tfinger.y * screen_height;
    if (x<(rect.x + rect.w) & x> rect.x & y<(rect.y + rect.h) & y> rect.y)
    {
      //printf("start sel=%i\n", selected);
      if (selected == 1)
      {
        // printf("sel=%i\n", selected);
        selected = 0;
      }
      else
      {
        // printf("notsel=%i\n", selected);
        selected = 1;
      }
      // printf("result sel=%i\n", selected);

      update();
    }
  }

  void update()
  {
    SDL_SetRenderTarget(renderer, button_tex); //set draw to texture not main renderer
                                               // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
    temprect = (SDL_Rect){0, 0, rect.w, rect.h};
    SDL_RenderDrawRect(renderer, &temprect);
    temprect = (SDL_Rect){1, 1, rect.w - 2, rect.h - 2};
    if (selected == 1)
    {
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 60);
    }
    else
    {
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
    }
    SDL_RenderFillRect(renderer, &temprect);
    font_tex = font->CreateFontTexture("Button", renderer, (SDL_Color){255, 255, 255, 128});

    SDL_QueryTexture(font_tex, NULL, NULL, &texW, &texH);
    //printf("rect w%i h%i texw%i texh%i  ox%i,oy%i\n", rect.w, rect.h, texW, texH, (rect.w - texW) / 2, (rect.h - texH) / 2);
    //  int w = texW; // * scale;
    // int h = texH; // * scale;

    font_rect = {(rect.w - texW) / 2, (rect.h - texH) / 2, texW, texH};
    SDL_RenderCopy(renderer, font_tex, NULL, &font_rect);
    SDL_SetRenderTarget(renderer, NULL); //set draw to texture not main renderer
  }

  void draw()
  {
    SDL_RenderCopy(renderer, button_tex, NULL, &rect);
  }

  int get_selected()
  {

    return selected;
  }
};

///////////////////////end of classes

int main()
{

  Display display = {};
  display.Init();
  SDL_SetRenderDrawBlendMode(display.main_renderer, SDL_BLENDMODE_BLEND);

  int main_button_width = screen_width / 8;
  int main_button_height = screen_width / 12;
  int main_button_txtsize = main_button_width / 5;
  int freq_display_width = (float)screen_width * 0.75;
  int freq_display_height = screen_height / 8;
  int freq_display_txtsize = freq_display_width / 13;

  Font button_font = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", main_button_txtsize}; //size=pixles, auto resized.
  Font digits_font = {"/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", freq_display_txtsize};
  Font slider_font = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", main_button_txtsize/2}; //size=pixles, auto resized.

  std::vector<Slider> sliders;
  sliders.push_back((Slider){display.main_renderer, (SDL_Rect){10, 200, 30, 200}, false, 0.8});
  sliders.push_back((Slider){display.main_renderer, (SDL_Rect){50, 200, 30, 200}, false, 0.1});
  sliders.push_back((Slider){display.main_renderer, (SDL_Rect){90, 200, 30, 200}, false, 0.5});
  sliders.push_back((Slider){display.main_renderer, (SDL_Rect){130, 200, 200, 30}, true, 0.65});

  //Layout freqlayout = {screen_width / 2, screen_height / 5, 550, 100};
  //Layout btnlayout = {screen_width / 2, screen_height - (screen_height / 8 / 2), screen_width - 2, screen_height / 8};

  std::vector<Button> buttons;

  for (int i = 0; i < 5; i++)
  {
    buttons.push_back((Button){display.main_renderer, (SDL_Rect){i * (screen_width / 8) + (i * 2), screen_height - (screen_height / 12), screen_width / 8, screen_height / 12}, &button_font});
  }

  FreqControl freq = {display.main_renderer, (SDL_Rect){(screen_width / 2) - (freq_display_width / 2), 40, freq_display_width, freq_display_height}, &digits_font};

  int x = 0;
  int y = 0;
  while (1)
  {

    SDL_SetRenderDrawColor(display.main_renderer, 5, 5, 5, 255);
    SDL_RenderClear(display.main_renderer);


    SDL_Event e;
    if (SDL_PollEvent(&e) != 0)   //SDL_WaitEvent
    {
      if (e.type == SDL_QUIT)
      {
        break;
      }

      if (e.type == SDL_FINGERMOTION)
      {
        for (Slider s : sliders)
        {
          s.update_gui(e);
        }
        // printf("touch.. x:%f y:%f\n", e.tfinger.x, e.tfinger.y);
      }

      if (e.type == SDL_FINGERDOWN)
      {
        for (Button &b : buttons)
        {
          b.update_gui(e);
        }
        freq.update_gui(e);
      }
    }

    for (Slider &s : sliders)
    {
      s.draw();
    }
    for (Button &b : buttons)
    {
      b.draw();
      //printf("selected=%i\n", b.get_selected());
    }
    freq.draw();

    SDL_RenderPresent(display.main_renderer);
    SDL_Delay(5);
  }
}
