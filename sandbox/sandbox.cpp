// sandbox.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "BTree.h"
//#include "mybtree.h"
//#include "DRAM.h"
#include <algorithm>    // std::lower_bound, std::upper_bound, std::sort
#include <vector>       // std::vector

#include <iostream>
#include <unordered_map>

class LRUCache {
private:
    struct Node {
        int key;
        std::shared_ptr<std::string> value;
        Node* prev;
        Node* next;

        Node(int k, std::string str) :  prev(nullptr), next(nullptr) 
        {
            key = k;
            value = std::make_shared<std::string>(str);
        }
    };

    int capacity;
    std::unordered_map<int, Node*> cache;
    Node* head;
    Node* tail;

    void moveToFront(Node* node) {
        if (node == head) {
            return; // Already at the front
        }

        if (node == tail) {
            tail = node->prev;
            tail->next = nullptr;
        }
        else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }

        node->prev = nullptr;
        node->next = head;
        head->prev = node;
        head = node;
    }

public:
    LRUCache(int capacity) : capacity(capacity), head(nullptr), tail(nullptr) {}

    std::shared_ptr<std::string> get(int key) {
        if (cache.find(key) != cache.end()) {
            Node* node = cache[key];
            moveToFront(node);
            return node->value;
        }
        else {
            return nullptr; // Not found
        }
    }

    void put(int key, std::string value) {
        if (cache.find(key) != cache.end()) {
            // Update existing key
            Node* node = cache[key];
            *node->value = value;
            moveToFront(node);
        }
        else {
            // Insert new key
            Node* newNode = new Node(key, value);
            if (cache.size() == capacity) {

                if (tail->value.use_count() > 1) {
                    std::cout << "tail is still in use.." << std::endl;
                    return;
                }

                // Remove the least recently used item
                cache.erase(tail->key);
                Node* temp = tail;
                tail = tail->prev;
                if (tail) {
                    tail->next = nullptr;
                }
                else {
                    head = nullptr;
                }
                delete temp;
            }

            cache[key] = newNode;
            if (!head) {
                head = newNode;
                tail = newNode;
            }
            else {
                newNode->next = head;
                head->prev = newNode;
                head = newNode;
            }
        }
    }
};

int main() {
    LRUCache cache(2);

    cache.put(1, "1");

    std::shared_ptr<std::string> _ptr = cache.get(1);

    cache.put(2, "2");
    cache.put(22, "22");
    std::cout << cache.get(1) << std::endl; // Output: 1

    _ptr.reset();

    cache.put(3, "3"); // evicts key 2
    std::cout << cache.get(2) << std::endl; // Output: -1 (not found)
    cache.put(4, "4"); // evicts key 1
    std::cout << cache.get(1) << std::endl; // Output: -1 (not found)
    std::cout << cache.get(3) << std::endl; // Output: 3
    std::cout << cache.get(4) << std::endl; // Output: 4

    return 0;
}

//int main()
//{
//   /* BPlusTree<int, int>* tree = new BPlusTree<int, int>(5);
//
//    for (size_t i = 50; i < 100; i++)
//    {
//        tree->insert(i, i);
//    }
//
//    int val = tree->search(1);
//    tree->remove(1);*/
//
//    //int myints[] = { 1, 2, 6};
//    //std::vector<int> v(myints, myints + 3);           // 10 20 30 30 20 10 10 20
//
//    //std::vector<int>::iterator low, up;
//    //low = std::lower_bound(v.begin(), v.end(), 7); //          ^
//    //up = std::upper_bound(v.begin(), v.end(), 7); //                   ^
//
//    //std::cout << "lower_bound at position " << *low << '\n';
//    //std::cout << "upper_bound at position " << *up << '\n';
//
//    //v.insert(low, 2);
//
//    //return 0;
//
//
//    std::cout << "Hello World!\n";
//
//    //BTree<int, int>* btree = new BTree<int, int>();
//
//    //btree->init();
//
//    ////btree->
//
//    //btree->print();
//}
//
//// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
//// Debug program: F5 or Debug > Start Debugging menu
//
//// Tips for Getting Started: 
////   1. Use the Solution Explorer window to add/manage files
////   2. Use the Team Explorer window to connect to source control
////   3. Use the Output window to see build output and other messages
////   4. Use the Error List window to view errors
////   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
////   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
