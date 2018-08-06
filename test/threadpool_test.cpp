#include "catch.hpp"
#include "../threadpool/Threadpool.hpp"

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>

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
namespace {
	// How long we expect that afterwards the other thread should have completed a task.
	const std::chrono::milliseconds THREAD_WAIT_MILLIS(200);

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
		};
		std::string addMe;
	};

	int intFunc() { return 4; }
	bool boolFunc() { return true; }

	struct info {
		double time; int id; std::string name;
		info() = default;
		info(double time, int id, const std::string& name) : time(time), id(id), name(name) {}
		bool operator==(const info& o) const { return time == o.time && id == o.id && name == o.name; }
	};
	info  makeInfo(double time, int id, const std::string& name) {
		return info{ time, id, name };
	}
	std::unique_ptr<info> makeUniqueInfo(double time, int id, const std::string& name) {
		return std::make_unique<info>(time, id, name);
	}
	int addFunc(int a, int b) { return a + b; }

	void waitFunc() {
		std::this_thread::sleep_for(THREAD_WAIT_MILLIS);
	}
}

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
		WHEN("A void functor taking no arguments is added by value.") {
			voidFunctor func;
			auto result = pool.add(func);
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
			}
		}
		WHEN("A void functor taking no arguments is added by reference.") {
			voidFunctor func;
			auto result = pool.add(std::ref(func));
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
			}
		}
		WHEN("A void functor taking no arguments is added by copy constructor.") {
			voidFunctor func;
			auto result = pool.add(voidFunctor(func));
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
			}
		}
		WHEN("A void functor taking no arguments is added by move.") {
			voidFunctor func;
			auto result = pool.add(std::move(func));
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
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
				CHECK(output[0] == "three");
				CHECK(output[1] == "two");
				CHECK(output[2] == "one");
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
		WHEN("A void functor with arguments is added by value.") {
			std::string param("hello ");
			voidFunctorWithParam functor{ "world" };
			auto result = pool.add(functor, std::ref(param));
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
				REQUIRE(param == "hello world");
			}
		}
		WHEN("A void functor with arguments is added by reference.") {
			std::string param("hello ");
			voidFunctorWithParam functor{ "world" };
			auto result = pool.add(std::ref(functor), std::ref(param));
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
				REQUIRE(param == "hello world");
			}
		}
		WHEN("A void functor with arguments is added by copy constructor.") {
			std::string param("hello ");
			voidFunctorWithParam functor{ "world" };
			auto result = pool.add(voidFunctorWithParam(functor), std::ref(param));
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
				REQUIRE(param == "hello world");
			}
		}
		WHEN("A void functor with arguments is added by move.") {
			std::string param("hello ");
			voidFunctorWithParam functor{ "world" };
			auto result = pool.add(std::move(functor), std::ref(param));
			THEN("It accepts it, and completes the job successfully.") {
				result.get();
				REQUIRE(param == "hello world");
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
		WHEN("A function returning a boolean with no arguments is added.") {
			auto result = pool.add(boolFunc);
			THEN("It accepts it, and resturns the expected value.") {
				REQUIRE(result.get() == true);
			}
		}
		WHEN("A function returning a struct is added.") {
			auto result = pool.add(makeInfo, 9.77, 10, "Happy Mannington");
			THEN("It accepts it, and returns the expected value.") {
				REQUIRE(result.get() == info{ 9.77, 10, "Happy Mannington" });
			}
		}
		WHEN("A function is added through a bind that returns a struct.") {
			auto result = pool.add(std::bind(makeInfo, 9.77, 10, "Happy Mannington"));
			THEN("It accepts it, and returns the expected value.") {
				REQUIRE(result.get() == info{ 9.77, 10, "Happy Mannington" });
			}
		}
		WHEN("A function returning a unique_ptr is added.") {
			auto result = pool.add(std::bind(makeUniqueInfo, 9.77, 10, "Happy Mannington"));
			THEN("It accepts it, and returns the expected value.") {
				REQUIRE((*result.get()) == info{ 9.77, 10, "Happy Mannington" });
			}
		}
	}
}

SCENARIO("A threadpool is given several functions at the same time.", "[threadpool][add]") {
	GIVEN("A default-initialized threadpool.") {
		Threadpool pool;

		WHEN("Two functions are added in succession.") {
			auto result1 = pool.add(boolFunc);
			auto result2 = pool.add(intFunc);
			THEN("It accepts them both, and completes them.") {
				CHECK(result1.get() == true);
				CHECK(result2.get() == 4);
			}
		}
		WHEN("Many functions are added sequentially.") {
			const int numFuncs = 10000;
			std::vector<std::future<int>> results;
			results.reserve(numFuncs);
			for (int i = 0; i < numFuncs; ++i) {
				results.push_back(pool.add(addFunc, i, 1));
			}
			AND_WHEN("It waits for all jobs to finish.") {
				pool.waitOnAllJobs();
				THEN("They are all completed successfully.") {
					for (int i = 0; i < numFuncs; ++i) {
						CHECK(results[i].get() == i + 1);
					}
				}
			}
		}
	}
}

SCENARIO("A threadpool is checked whether it is idle.", "[threadpool][isIdle]") {
	GIVEN("A default-initialized threadpool.") {
		Threadpool pool;

		WHEN("A function that takes some time to complete is added.") {
			pool.add(waitFunc);
			THEN("The threadpool is not idle until after it completes.") {
				CHECK_FALSE(pool.isIdle());
				pool.waitOnAllJobs();
				CHECK(pool.isIdle());
			}
		}
		WHEN("A function that returns immediately is added.") {
			pool.add(voidFunc);
			THEN("After a short wait, the threadpool should be idle.") {
				std::this_thread::sleep_for(THREAD_WAIT_MILLIS);
				CHECK(pool.isIdle());
			}
		}
	}
}
