#include <vk_engine.h>
#undef main // fixes linker error with sdl

int main(int argc, char* argv[])
{
	VulkanEngine engine;

	engine.init();	
	
	engine.run();

	engine.cleanup();	

	return 0;
}
