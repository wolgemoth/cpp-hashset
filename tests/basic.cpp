#include "../Hashset.hpp"

#include <iostream>
#include <cassert>
#include <string>

/**
 * @file basic.cpp
 * @brief Basic tests for the functionality of the hashset.
 */
int main([[maybe_unused]] int _argc, [[maybe_unused]] char* _argv[]) {

	LouiEriksson::Hashset<int> hashset;

	std::cout << "~ BASIC TESTS ~\n";

	// Test 1: Insertion
	{
		std::cout << "Test 1: Insertion..." << std::flush;

		hashset.Add(1);
		hashset.Add(2);
		hashset.Add(3);

		std::cout << "Done.\n";
	}

	// Test 2: Existence check
	{
		std::cout << "Test 2: Existence check..." << std::flush;

		assert( hashset.ContainsKey(1) && "Failed on key 1.");
		assert( hashset.ContainsKey(2) && "Failed on key 2.");
		assert( hashset.ContainsKey(3) && "Failed on key 3.");
		assert(!hashset.ContainsKey(4) && "Failed on key 4.");

		std::cout << "Done.\n";
	}

	// Test 3: Value retrieval
	{
		std::cout << "Test 3: Value retrieval..." << std::flush;

		auto item1 = hashset.Get(1);
		auto item2 = hashset.Get(2);
		auto item3 = hashset.Get(3);

		assert((item1.value() == 1) && "Failed on key 1.");
		assert((item2.value() == 2) && "Failed on key 2.");
		assert((item3.value() == 3) && "Failed on key 3.");

		std::cout << "Done.\n";
	}

	// Test 5: Deletion
	{
		std::cout << "Test 5: Deletion..." << std::flush;

		hashset.Remove(1);

		assert(!hashset.ContainsKey(1) && "Failed on key 1.");

		std::cout << "Done.\n";
	}

	// Test 6: Clearing
	{
		std::cout << "Test 5: Clearing..." << std::flush;

		hashset.Clear();

		assert(hashset.size() == 0 && "Failed on key 1.");

		std::cout << "Done.\n";
	}

	std::cout << "All tests passed!\n";

	return 0;
}