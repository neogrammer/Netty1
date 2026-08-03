#include <res/Cfg.h>

ResourceManager<sf::Texture, int> Cfg::textures = {};
ResourceManager<sf::Font, int> Cfg::fonts = {};


void Cfg::Initialize()
{
    initTextures();
    initFonts();
}


void Cfg::initTextures()
{
	textures.load((int)Textures::Hero1_Sheet, "assets/textures/player/Hero1_Sheet_Big.png");
    textures.load((int)Textures::None, "assets/textures/none.png");

}


void Cfg::initFonts()
{
    fonts.load((int)Fonts::Bubbly, "assets/fonts/bubbly.ttf");
}