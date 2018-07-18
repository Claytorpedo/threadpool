#include "catch.hpp"
#include "../threadpool/Threadpool.hpp"

SCENARIO("A threadpool is constructed." "[threadpool][add]") {
	WHEN("A threadpool is initialized with default parameters.") {
		Threadpool pool;
	}
	WHEN("A threadpool is initialized with no threads.") {
		Threadpool pool(0, 0, 0);

		// can't add jobs or something?
	}
}
