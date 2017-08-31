
#include "define.h"
#include "game.h"
#include "client.h"

int main(int argc, char* argv[] ) {
	argv = uv_setup_args(argc, argv);

	// 初始化loop
	uv_loop_t* loop = uv_default_loop();

	int ret = InitGame("127.0.0.1", 12345);
	if (ret != 1) return 0;

	

	// 启动循环
	uv_run(loop, UV_RUN_DEFAULT);
	return 1;
}