#include "catch.hpp"
#include "../threadpool/Threadpool.hpp"

SCENARIO("A threadpool is constructed.", "[threadpool][construction]") {
	WHEN("A threadpool is initialized with default parameters.") {
		Threadpool pool;
	}
	WHEN("A threadpool is initialized with no threads.") {
		Threadpool pool(0, 0, 0);
	}
	WHEN("A threadpool is initialized with many threads.") {
		Threadpool pool(1000);
	}
}

void voidFunc() {};

SCENARIO("A threadpool is given various functions.", "[threadpool][add]") {
	GIVEN("A default-initialized threadpool.") {
		Threadpool pool;
		WHEN("A void function taking no arguments is added.") {
			auto result = pool.add(voidFunc);
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
			}
		}
	}
}

