#include"Logger.h"
void log_test()
{

    std::thread t1([]()
                   {
		for (int i = 0; i < 900000; ++i)
			LOG_INFO << "1111woshisdfsd " << 23 << 34 << "buox"; });
    t1.detach();
    std::thread t2([]()
                   {
		for (int i = 0; i < 1010000; ++i)
			//LOG_DEBUG << "22woshisdfsd " << 23 << 34 << "buox";
			LOG_INFO << "wo是log_info的" << 555 << " test"; });
    t2.detach();
    for (int i = 0; i < 1010000; ++i)
        LOG_INFO << 11 << "中国人" << 23;
    std::this_thread::sleep_for(std::chrono::seconds(3));
}
int main(int argc, char const *argv[])
{
    log_test();
    return 0;
}
