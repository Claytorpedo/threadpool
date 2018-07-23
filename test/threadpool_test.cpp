#include "catch.hpp"
#include "../threadpool/Threadpool.hpp"

#include <vector>
#include <string>

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

// ------------------ Test functions/functors ----------------------
void voidFunc() {}
void getVal(int* val) { *val = 8; }
void reverseCopyVec(int numItems, const std::vector<std::string>& input, std::vector<std::string>& output) {
	for (int i = numItems; i > 0; ) {
		output.push_back(input[--i]);
	}
}
struct voidFunctor { void operator()() const {} };
struct voidFunctorWithParam {
	void operator()(std::string& str) {
		str += addMe;
		addMe = str;
	};
	std::string addMe;
};

int intFunc() { return 4; }
// -----------------------------------------------------------------

SCENARIO("A threadpool is given various void functions.", "[threadpool][add][void]") {
	GIVEN("A default-initialized threadpool.") {
		Threadpool pool;

		WHEN("A void function taking no arguments is added.") {
			auto result = pool.add(voidFunc);
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
			}
		}
		WHEN("A void lambda taking no arguments is added.") {
			auto result = pool.add([]() {});
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
			}
		}
		WHEN("A void functor taking no arguments is added.") {
			voidFunctor func;
			auto result1 = pool.add(func);              // By value.
			auto result2 = pool.add(std::ref(func));    // By reference;
			auto result3 = pool.add(voidFunctor(func)); // By copy constructor.
			auto result4 = pool.add(std::move(func));   // By move.
			THEN("It accepts it, and completes the job successfully.") {
				result1.get();
				result2.get();
				result3.get();
				result4.get();
			}
		}
		WHEN("A void function with one argument is added.") {
			int i = 0;
			auto result = pool.add(getVal, &i);
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
				REQUIRE(i == 8);
			}
		}
		WHEN("A void function with one argument is added through a bind.") {
			int i = 0;
			auto result = pool.add(std::bind(getVal, &i));
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
				REQUIRE(i == 8);
			}
		}
		WHEN("A void function with several arguments is added.") {
			std::vector<std::string> input = { "one", "two", "three" };
			std::vector<std::string> output;
			auto result = pool.add(reverseCopyVec, input.size(), std::ref(input), std::ref(output));
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
				REQUIRE(output[0] == "three");
				REQUIRE(output[1] == "two");
				REQUIRE(output[2] == "one");
			}
		}
		WHEN("A void lambda taking one argument is added.") {
			int i = 0;
			auto result = pool.add([&i]() { i = 42; });
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
				REQUIRE(i == 42);
			}
		}
	}
}

SCENARIO("A threadpool is given various functions with return values.", "[threadpool][add][non-void]") {
	GIVEN("A default-initialized threadpool.") {
		Threadpool pool;

		WHEN("A function returning an int with no arguments is added.") {
			auto result = pool.add(intFunc);
			THEN("It accepts it, and returns the expected value.") {
				REQUIRE(result.get() == 4);
			}
		}
	}
}
