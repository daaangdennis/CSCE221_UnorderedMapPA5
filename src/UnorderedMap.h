#include <cstddef>    // size_t
#include <functional> // std::hash
#include <ios>
#include <utility>    // std::pair
#include <iostream>

#include "primes.h"



template <typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>>
class UnorderedMap {
    public:

    using key_type = Key;
    using mapped_type = T;
    using const_mapped_type = const T;
    using hasher = Hash;
    using key_equal = Pred;
    using value_type = std::pair<const key_type, mapped_type>;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    private:

    struct HashNode {
        HashNode *next;
        value_type val;

        HashNode(HashNode *next = nullptr) : next{next} {}
        HashNode(const value_type & val, HashNode * next = nullptr) : next { next }, val { val } { }
        HashNode(value_type && val, HashNode * next = nullptr) : next { next }, val { std::move(val) } { }
    };

    size_type _bucket_count;
    HashNode **_buckets;

    HashNode * _head;
    size_type _size;

    Hash _hash;
    key_equal _equal;

    static size_type _range_hash(size_type hash_code, size_type bucket_count) {
        return hash_code % bucket_count;
    }

    public:

    template <typename pointer_type, typename reference_type, typename _value_type>
    class basic_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = _value_type;
        using difference_type = ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

    private:
        friend class UnorderedMap<Key, T, Hash, key_equal>;
        using HashNode = typename UnorderedMap<Key, T, Hash, key_equal>::HashNode;

        const UnorderedMap * _map;
        HashNode * _ptr;

        explicit basic_iterator(UnorderedMap const * map, HashNode *ptr) noexcept { _map = map; _ptr = ptr; }

    public:
        basic_iterator() { _map = nullptr; _ptr = nullptr;};

        basic_iterator(const basic_iterator &) = default;
        basic_iterator(basic_iterator &&) = default;
        ~basic_iterator() = default;
        basic_iterator &operator=(const basic_iterator &) = default;
        basic_iterator &operator=(basic_iterator &&) = default;
        reference operator*() const { return _ptr->val; }
        pointer operator->() const { return &(_ptr->val); }
        basic_iterator &operator++() { 
            if(_ptr == nullptr)
            {
                return *this;
            }

            if(_ptr->next != nullptr)
            {
                _ptr = _ptr->next;
                return *this;
            }

            size_type currentBucket = _range_hash(_map->_hash(_ptr->val.first), _map->_bucket_count);
            for(size_type currentIndex = currentBucket+1; currentIndex < _map->_bucket_count; currentIndex++)
            {
                HashNode* bucket = _map->_buckets[currentIndex];
                if(bucket != nullptr)
                {
                    _ptr = bucket;
                    return *this;
                }
            }

            _ptr = nullptr;
            return *this;
        }
        basic_iterator operator++(int) {
            basic_iterator temp = *this;
            ++(*this);
            return temp;
        }
        bool operator==(const basic_iterator &other) const noexcept { return this->_ptr == other._ptr; }
        bool operator!=(const basic_iterator &other) const noexcept { return this->_ptr != other._ptr;  }
    };

    using iterator = basic_iterator<pointer, reference, value_type>;
    using const_iterator = basic_iterator<const_pointer, const_reference, const value_type>;

    class local_iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<const key_type, mapped_type>;
            using difference_type = ptrdiff_t;
            using pointer = value_type *;
            using reference = value_type &;

        private:
            friend class UnorderedMap<Key, T, Hash, key_equal>;
            using HashNode = typename UnorderedMap<Key, T, Hash, key_equal>::HashNode;

            HashNode * _node;

            explicit local_iterator( HashNode * node ) noexcept { _node = node; }

        public:
            local_iterator() { _node = nullptr; }

            local_iterator(const local_iterator &) = default;
            local_iterator(local_iterator &&) = default;
            ~local_iterator() = default;
            local_iterator &operator=(const local_iterator &) = default;
            local_iterator &operator=(local_iterator &&) = default;
            reference operator*() const { return _node->val; }
            pointer operator->() const { return &(_node->val);}
            local_iterator & operator++() { 
                if(_node == nullptr)
                {
                    return *this;
                }
                else if(_node->next == nullptr)
                {
                    _node = nullptr;
                    return *this;
                }
                _node = _node->next;
                return *this;
            }
            local_iterator operator++(int) { 
                local_iterator temp = *this;
                ++(*this);
                return temp;
            }

            bool operator==(const local_iterator &other) const noexcept { return this->_node == other._node; }
            bool operator!=(const local_iterator &other) const noexcept { return this->_node != other._node; }
    };

private:

    size_type _bucket(size_t code) const { return _range_hash(code, _bucket_count); }
    size_type _bucket(const Key & key) const { return _range_hash(_hash(key), _bucket_count); }
    size_type _bucket(const value_type & val) const { return _range_hash(_hash(val.first), _bucket_count); }

    HashNode*& _find(size_type code, size_type bucket, const Key & key) { 
        
        HashNode** currentNode = &(_buckets[bucket]);

        while(*currentNode != nullptr) // Loop through bucket list
        {
            if(_equal((*currentNode)->val.first, key)) // Return node if found
            {
                return *currentNode;
            }
            currentNode = &((*currentNode)->next);
        }

        return *currentNode; // Return nullptr if not found
    }

    HashNode*& _find(const Key & key) { return _find(_hash(key), _bucket(key), key); }

    HashNode * _insert_into_bucket(size_type bucket, value_type && value) {
        HashNode* bucketHead = _buckets[bucket]; // Get bucket head
        HashNode* nodeToInsert = new HashNode(std::move(value)); // Create new hash node with value

        if(bucketHead == nullptr) // If bucket is empty, add in new node
        {
            nodeToInsert->next = nullptr;
            _buckets[bucket] = nodeToInsert;
            _size++;
        }
        else // If bucket is not empty, set nodeToInsert->next to current bucket head and make new bucket head nodeToInsert
        {
            nodeToInsert->next = bucketHead;
            _buckets[bucket] = nodeToInsert;
            _size++;
        }

        // Assign the node being inserted to _head if there is no current head or if it is in an earlier bucket
        if(_head == nullptr)
        {
            _head = nodeToInsert;
        }
        else if(_bucket(value) <= _bucket(_head->val))
        {
            _head = nodeToInsert;
        }
        return nodeToInsert; // Return node that was inserted
    }

    void _move_content(UnorderedMap & src, UnorderedMap & dst) {
        
        // Transfer everything to dst
        
        dst._bucket_count = src._bucket_count;
        dst._buckets = new HashNode*[src._bucket_count];
        dst._buckets = src._buckets;
        dst._head = src._head;
        dst._size = src._size;
        dst._hash = std::move(src._hash);
        dst._equal = std::move(src._equal);

        // Clear src data
        src._bucket_count = 0;
        src._buckets = nullptr;
        src._head = nullptr;
        src._size = 0;
    }

public:
    explicit UnorderedMap(size_type bucket_count, const Hash & hash = Hash { },
                const key_equal & equal = key_equal { }) 
                { 
                    _bucket_count = next_greater_prime(bucket_count);
                    _buckets = new HashNode*[_bucket_count]();
                    _size = 0;
                    _head = nullptr;
                }

    ~UnorderedMap() { /* TODO */ }

    UnorderedMap(const UnorderedMap & other){ 
        // Set initial values
        _bucket_count = other._bucket_count;
        _buckets = new HashNode*[_bucket_count]();
        _head = nullptr;
        _size = 0;
        _hash = other._hash;
        _equal = other._equal;

        // Copy all nodes over
        for(size_type bucketIndex = 0; bucketIndex < other._bucket_count; bucketIndex++)
        {
            HashNode* currentNode = other._buckets[bucketIndex];
            while(currentNode != nullptr)
            {
                insert(currentNode->val);
                currentNode = currentNode->next;
            }
        }

    }

    UnorderedMap(UnorderedMap && other) { _move_content(other, *this); }

    UnorderedMap & operator=(const UnorderedMap & other) { /* TODO */ }

    UnorderedMap & operator=(UnorderedMap && other) { /* TODO */ }

    void clear() noexcept { /* TODO */ }

    size_type size() const noexcept { return _size; }

    bool empty() const noexcept { return _size == 0;  }

    size_type bucket_count() const noexcept { return _bucket_count; }

    iterator begin() { }
    iterator end() { }

    const_iterator cbegin() const { /* TODO */ };
    const_iterator cend() const { /* TODO */ };

    local_iterator begin(size_type n) { return local_iterator(_buckets[n]);}
    local_iterator end(size_type n) { return local_iterator(nullptr);}

    size_type bucket_size(size_type n) { 
        size_type count = 0;
        HashNode* currentNode = _buckets[n];

        while(currentNode != nullptr)
        {
            count++;
            currentNode = currentNode->next;
        }
        return count;
    }

    float load_factor() const { return ((float)_size)/_bucket_count; }

    size_type bucket(const Key & key) const { return _bucket(key); }

    std::pair<iterator, bool> insert(value_type && value) {    
        HashNode* possibleDup = _find(value.first); // Look to see if key already exists
        
        if(possibleDup == nullptr) // If key doesnt exist, proceed with insert
        {
            return std::make_pair(iterator(this, _insert_into_bucket(_bucket(value), std::move(value))), true);
        }
        return std::make_pair(iterator(this, possibleDup), false);
    }

    std::pair<iterator, bool> insert(const value_type & value) { 
        HashNode* possibleDup = _find(value.first); // Look to see if key already exists
        
        if(possibleDup == nullptr) // If key doesnt exist, proceed with insert
        {
            HashNode* bucketHead = _buckets[_bucket(value)]; // Get bucket head
            HashNode* nodeToInsert = new HashNode(value); // Make new hash node

            if(bucketHead == nullptr) // Case for when bucket is empty
            {
                nodeToInsert->next = nullptr;
                _buckets[_bucket(value)] = nodeToInsert;
                _size++;
            }
            else // Case for when bucket is not empty
            {
                nodeToInsert->next = bucketHead;
                _buckets[_bucket(value)] = nodeToInsert;
                _size++;
            }
            // Set _head as needed
            if(_head == nullptr)
            {
                _head = nodeToInsert;
            }
            else if(_bucket(value) <= _bucket(_head->val))
            {
                _head = nodeToInsert;
            }
            return std::make_pair(iterator(this, nodeToInsert), true);
        }
        
        return std::make_pair(iterator(this, possibleDup), false);

    }

    iterator find(const Key & key) { /* TODO */ }

    T& operator[](const Key & key) { /* TODO */ }

    iterator erase(iterator pos) { /* TODO */ }

    size_type erase(const Key & key) { /* TODO */ }

    template<typename KK, typename VV>
    friend void print_map(const UnorderedMap<KK, VV> & map, std::ostream & os);
};

template<typename K, typename V>
void print_map(const UnorderedMap<K, V> & map, std::ostream & os = std::cout) {
    using size_type = typename UnorderedMap<K, V>::size_type;
    using HashNode = typename UnorderedMap<K, V>::HashNode;

    for(size_type bucket = 0; bucket < map.bucket_count(); bucket++) {
        os << bucket << ": ";

        HashNode const * node = map._buckets[bucket];

        while(node) {
            os << "(" << node->val.first << ", " << node->val.second << ") ";
            node = node->next;
        }

        os << std::endl;
    }
}
