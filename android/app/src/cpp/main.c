
#include <jni.h>

#include <android/log.h>
#include <android_native_app_glue.h>
#include <android_native_app_glue.c>

#include <benchmark.h>

void android_main(struct android_app * app)
{
	LOGI("hello\n");

	double perf_1024 = benchmark(1024, 1000);
	double perf_16384 = benchmark(16384, 50);

	LOGI("results for context backend:\n");
	LOGI("%u: %.1f switches/sec\n", 1024, perf_1024);
	LOGI("%u: %.1f switches/sec\n", 16384, perf_16384);
	return;
}
