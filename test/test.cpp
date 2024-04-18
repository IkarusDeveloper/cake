#define TEST_OWNER_THREAD_SAFE
#include <iostream>
#include <functional>
#include <set>
#include <unordered_set>
#ifdef TEST_OWNER_THREAD_SAFE
    #include "../include/cake/owner_ts.h"
    #include <thread>
    #include <random>
#else
    #include "../include/cake/owner.h"
#endif

int g_passed_test_count = 0;
int g_failed_test_count = 0;

#define TEST(function) execute_single_test(#function, function)
#define TEST_ASSERT(condition)                                       \
    if (!(condition)) {                                              \
        std::cout << "ASSERTION FAILED ON LINE " << __LINE__ << ": " \
                  << #condition << std::endl;                        \
        return false;                                                \
    }

void execute_single_test(const std::string& name, std::function<bool()> func) {
    if (!func()) {
        std::cout << "FAILED TEST : " << name << std::endl;
        g_failed_test_count++;
        return;
    }
    g_passed_test_count++;
}

bool test_owner_ptr() {
    // making an owner_ptr
    auto o1 = cake::make_owner<std::string>("prettystring");
    TEST_ASSERT(o1.get() != nullptr);
    TEST_ASSERT(o1.alive());
    TEST_ASSERT(*o1 == "prettystring");

    // making a copy
    auto copy_o1 = o1;
    TEST_ASSERT(copy_o1.get() != nullptr);
    TEST_ASSERT(copy_o1.alive());
    TEST_ASSERT(*copy_o1 == "prettystring");

    // destroying internal object
    copy_o1.owner_delete();
    TEST_ASSERT(o1.get() == nullptr);
    TEST_ASSERT(!o1.alive());

    return true;
}

bool test_weak_ptr() {
    cake::weak_ptr<std::string> weak;
    // making owner in another scope
    {
        // making an owner_ptr
        auto o1 = cake::make_owner<std::string>("prettystring");

        // making the weak ptr
        weak = cake::make_weak(o1);
        TEST_ASSERT(weak.get() != nullptr);
        TEST_ASSERT(weak.alive());
        TEST_ASSERT(*weak == "prettystring");

        // making a copy of the weak ptr
        auto weak2 = weak;
        TEST_ASSERT(weak2.get() != nullptr);
        TEST_ASSERT(weak2.alive());
        TEST_ASSERT(*weak2 == "prettystring");

        // taking back ownership from weak ptr
        auto o2 = cake::get_ownership(weak2);
        TEST_ASSERT(o2.get() != nullptr);
        TEST_ASSERT(o2.alive());
        TEST_ASSERT(*o2 == "prettystring");
    }

    // testing weak no longer alive
    TEST_ASSERT(weak.get() == nullptr);
    TEST_ASSERT(!weak.alive());

    return true;
}

bool test_proxy_ptr() {
    struct proxable_string_base {};

    struct proxable_string
        : public proxable_string_base,
          public cake::enable_proxy_from_this<proxable_string> {
        std::string str;
    };

    proxable_string string;
    string.str = "prettystring";

    // making a proxy
    auto ps = string.proxy();
    TEST_ASSERT(ps.get() != nullptr);
    TEST_ASSERT(ps.alive());
    TEST_ASSERT(ps->str == "prettystring");

    // making a copy of the proxy
    auto copy_ps = ps;
    TEST_ASSERT(copy_ps.get() != nullptr);
    TEST_ASSERT(copy_ps.alive());
    TEST_ASSERT(copy_ps->str == "prettystring");

    // testing proxy_from_base
    auto bps = string.proxy_from_base<proxable_string_base>();
    TEST_ASSERT(bps.get() != nullptr);
    TEST_ASSERT(bps.alive());

    // getting back derived from base
    auto dps = cake::static_pointer_cast<proxable_string>(bps);

    // destroying all proxy
    string.proxy_delete();

    TEST_ASSERT(ps.get() == nullptr);
    TEST_ASSERT(!ps.alive());

    TEST_ASSERT(copy_ps.get() == nullptr);
    TEST_ASSERT(!copy_ps.alive());

    // testing for auto-deleting on destructor
    cake::proxy_ptr<proxable_string> psa;

    {
        // adding a new scope in order to call
        // destructor
        proxable_string string2;
        string2.str = "prettystring";
        psa = string2.proxy();

        TEST_ASSERT(psa.get() != nullptr);
        TEST_ASSERT(psa.alive());
        TEST_ASSERT(psa->str == "prettystring");
    }

    TEST_ASSERT(psa.get() == nullptr);
    TEST_ASSERT(!psa.alive());

    return true;
}

#ifdef TEST_OWNER_THREAD_SAFE
int random_integer(int min, int max) {
    static std::default_random_engine eng(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(eng);
}

bool test_owner_thread_safe() {
    // event that makes 10 threads waiting one of them to destroy the object
    auto func = []() {
        auto owner = cake::make_owner<std::string>("prettystring");
        auto weak = cake::make_weak(owner);
        std::vector<std::thread> threads;
        for (int i = 0; i < 10; i++) {
            auto thread_event = [&weak, &owner, index = i]() {
                // trying to delete via owner
                while (true) {
                    auto lock = owner.try_lock();
                    if (!lock)
                        return;

                    // trying to delete via owner
                    if (random_integer(0, 1000) == 5) {
                        /*std::cout << "deleting via owner: thread id " << index
                                  << std::endl;*/
                        owner.owner_delete();
                    }

                    // trying to delete via weak
                    else if (random_integer(0, 1000) == 6) {
                        /*std::cout << "deleting via weak: thread id " << index
                                  << std::endl;*/
                        if (auto temp_owner = cake::get_ownership(weak))
                            temp_owner.owner_delete();
                    }
                }
            };

            threads.emplace_back(std::thread(thread_event));
        }

        for (auto& thread : threads)
            thread.join();
    };

    for (int i = 0; i < 100; i++)
        func();

    return true;
}
#endif

bool test_cake_smart_pointers_key_set() {
    // inserting owner_ptr into a set
    auto owner_key = cake::make_owner<std::string>();
    auto test_owner_set = std::set<cake::owner_ptr<std::string>>();
    test_owner_set.emplace(owner_key);
    // destroying the object
    owner_key.owner_delete();
    // checking the key still work
    TEST_ASSERT(test_owner_set.contains(owner_key));

    // insert weak_ptr into a set
    auto obj = cake::make_owner<std::string>();
    auto weak = cake::make_weak(obj);
    std::set<cake::weak_ptr<std::string>> test_weak_set;
    test_weak_set.emplace(weak);
    // destroying the object
    obj.owner_delete();
    // checking the key still work
    TEST_ASSERT(test_weak_set.contains(weak));

    // insert proxy_ptr into a set
    struct proxable_integer : cake::enable_proxy_from_this<proxable_integer> {
        int value = 0;
    };
    std::set<cake::proxy_ptr<proxable_integer>> test_proxy_set;
    cake::proxy_ptr<proxable_integer> proxy_key;
    {
        proxable_integer value;
        proxy_key = value.proxy();
        test_proxy_set.emplace(proxy_key);
    }
    // checking the key still work
    TEST_ASSERT(test_proxy_set.contains(proxy_key));

    return true;
}

bool test_cake_smart_pointers_key_unordered_set() {
    // inserting owner_ptr into a set
    auto owner_key = cake::make_owner<std::string>();
    auto test_owner_set = std::unordered_set<cake::owner_ptr<std::string>>();
    test_owner_set.emplace(owner_key);
    // destroying the object
    owner_key.owner_delete();
    // checking the key still work
    TEST_ASSERT(test_owner_set.contains(owner_key));

    // insert weak_ptr into a set
    auto obj = cake::make_owner<std::string>();
    auto weak_key = cake::make_weak(obj);
    std::unordered_set<cake::weak_ptr<std::string>> test_weak_set;
    test_weak_set.emplace(weak_key);
    // destroying the object
    obj.owner_delete();
    // checking the key still work
    TEST_ASSERT(test_weak_set.contains(weak_key));

    // insert proxy_ptr into a set
    struct proxable_integer : cake::enable_proxy_from_this<proxable_integer> {
        int value = 0;
    };
    std::unordered_set<cake::proxy_ptr<proxable_integer>> test_proxy_set;
    cake::proxy_ptr<proxable_integer> proxy_key;
    {
        proxable_integer value;
        proxy_key = value.proxy();
        test_proxy_set.emplace(proxy_key);
    }
    // checking the key still work
    TEST_ASSERT(test_proxy_set.contains(proxy_key));

    return true;
}

bool test_proxy_parent_base_vector() {
    struct proxable_string : cake::enable_proxy_from_this<proxable_string> {
        // some compiler may require *move constructor* and *destructor* as
        // noexcept!
        /*proxable_string() = default;
        proxable_string(const proxable_string&) = default;
        proxable_string(proxable_string&&) noexcept = default;
        ~proxable_string() noexcept = default;*/
        std::string str;
    };

    // making vector with a size of 2
    std::vector<proxable_string> strings;
    strings.resize(2);

    // storing proxy values into another vector
    std::vector<cake::proxy_ptr<proxable_string>> proxy_vec;
    for (auto& string : strings)
        proxy_vec.emplace_back(string.proxy());

    // increaseing the vector size making it reallocating the elements
    strings.resize(2000);

    // checking all the proxy are keep alive
    auto alive_check = [](auto& proxy) { return proxy.alive(); };
    TEST_ASSERT(std::all_of(proxy_vec.begin(), proxy_vec.end(), alive_check));

    return true;
}

void print_test_results() {
    if (g_failed_test_count == 0)
        std::cout << "All tests are passed." << std::endl;
    else {
        std::cout << "Passed Tests: " << g_passed_test_count << std::endl;
        std::cout << "Failed tests: " << g_failed_test_count << std::endl;
    }
}

int main() {
    std::cout << "Starting the tests..." << std::endl;

    // executing all tests
    TEST(test_owner_ptr);
    TEST(test_weak_ptr);
    TEST(test_proxy_ptr);
#ifdef TEST_OWNER_THREAD_SAFE
    TEST(test_owner_thread_safe);
#endif
    TEST(test_cake_smart_pointers_key_set);
    TEST(test_cake_smart_pointers_key_unordered_set);
    TEST(test_proxy_parent_base_vector);

    // printing test results
    std::cout << "All tests executed." << std::endl;
    print_test_results();

    // pausing the program
    std::cout << "Press ENTER to close." << std::endl;
    std::cin.ignore();
    return 0;
}
