# extalloc
A storage allocator using external metadata. Many dynamic storage allocation scheme store their metadata --- the collection of allocated and free blocks --- within the blocks themselves. In contrast, extalloc stores this information externally: currently in a pair of `std::map<>` instances.
