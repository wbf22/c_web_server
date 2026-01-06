#ifndef MAP
#define MAP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct Element {
    char* key;
    size_t key_size;
    void* data;
} Element;

/*
    An unordered hash map implementation.

    Used like so:
    ```
    
    typedef struct MyStruct {
        int x;
        int y;
    } MyStruct;

    // insert into map
    Map* map = new_map();
    MyStruct object = {1, 3};
    m_unique(map, "my_key", &object);

    // update an element (2 ways)
    MyStruct* back_out = (MyStruct*) m_get(map, "my_key");
    back_out->y = 14;
    back_out = (MyStruct*) m_get(map, "my_key");
    printf("{%d,%d}\n",back_out->x, back_out->y);

    MyStruct replacement = {10, 33};
    m_put(map, "my_key", &replacement, sizeof(replacement));
    back_out = (MyStruct*) m_get(map, "my_key");
    printf("{%d,%d}\n",back_out->x, back_out->y);

    // remove element
    m_erase(map, "my_key");

    // get map length
    printf("%d\n", map->len);

    // iterate over map elements
    for (int i = 0; i < 2; i++) {
        MyStruct* new_object = malloc(sizeof(MyStruct));
        new_object->x = 1 + i;
        new_object->y = 3 + i;

        char key[5];
        make_key(&i, sizeof(int), key, 5);
        m_unique(map, key, new_object);
    }

    Element** items = map_elements(map);
    for (int i = 0; i < map->len; ++i) {
        Element* item = items[i];
        char* key = item->key;
        MyStruct obj = *(MyStruct*) item->data;

        printf("%s{%d,%d}\n", key, obj.x, obj.y);
    }

    // clean up
    for (int i = 0; i < map->len; ++i) {
        Element* item = items[i];
        MyStruct* obj = (MyStruct*) item->data;
        free(obj);
    }
    free(items); // map_elements allocates memory for the 'items' array
    free_map(map);


    ```

    The map does not free any items placed in it, so if you create
    objects on the heap and put them in the map, make sure to clean 
    them up yourself after calling 'free_map' or after erasing the items
    from the map.

    You can mix heap and stack items in the map as well as mixing types
    if you desire. 

    The map returns NULL if a value is not in the map instead of exiting the program
    or something.



    # METHODS

    Methods with their time complexity
    - m_put() m_int_put() m_any_put() m_unique() m_int_unique() m_any_unique() -> O(1) amoritized
    - m_get() m_int_get() m_any_get(() -> O(1) amoritized
    - m_erase() m_int_m_erase() m_any_m_erase() -> O(1) amoritized
    - free_map() -> O(n)
    - clear_map() -> O(n)
    - map_elements() -> O(n)
    - m_contains() m_int_contains() m_any_contains() -> O(1) amoritized



    # DESIGN
    
    The map uses a hash table (an array) with keys converted to indices
    with a hash function. We use djb2's hash function for this. 

    On hash collions we use probing with a double hash function to insert the element.
    When the hash table is 70% full we do a resize, using the next prime table size
    in a predefined primes list.



*/
typedef struct Map {
    Element** data; // pointer to array of pointers
    size_t data_size;
    size_t len;
} Map;

const char* DELETED_KEY = "<DELETED>";

const size_t PRIMES[] = {
    127,
    257,
    509,
    1021,
    2053,
    4099,
    9311,
    18253,
    37633,
    65713,
    115249,
    193939,
    505447,
    1062599,
    2017963,
    4393139,
    6972593,
    13466917,
    30402457,
    57885161,
    99990001,
    370248451,
    492366587,
    715827883,
    6643838879,
    8589935681,
    32212254719,
    51539607551, // 412 GB of 8 byte elements
    
    // just for fun probably
    80630964769,
    119327070011,
    228204732751,
    1171432692373,
    1398341745571,
    9809862296159,
    15285151248481,
    304250263527209,
    1746860020068409,
    10657331232548839,
    790738119649411319,
    2305843009213693951 // 18,446 PB of 8 byte elements, more than largest super computers have in ram
};

static void map_mem_error_exit_failing() {
    fprintf(stderr, "Map couldn't get more memory on the system! Exiting...");
    exit(EXIT_FAILURE);
}


static Map* new_map_s(size_t size) {

    Map *map = malloc(sizeof(Map));
    if (map == NULL) {
        map_mem_error_exit_failing();
    }
    map->data = malloc(size * sizeof(void*));
    if (map->data == NULL) {
        free(map);
        map_mem_error_exit_failing();
    }
    map->data_size = size;
    for (int i = 0; i < size; ++i) {
        map->data[i] = NULL;
    }
    map->len = 0;

    return map;
}

/*
    Creates an empty map
*/
Map* new_map() {
    return new_map_s(PRIMES[0]);
}

/*
    Function to hash an object's data into a key. Not necessary for use with
    this Map implementation, but can be handy. 

    Use like so:
    ```
    typedef struct MyStruct {
        int x;
        int y;
        float f;
    } MyStruct;

    MyStruct my_struct;
    my_struct.x = 1;
    my_struct.y = 2;
    my_struct.f = 19;

    char key[10]; 
    make_key(&my_struct, sizeof(my_struct), key, 10);
    printf("%s", key);
    ```
*/
void make_key(void* object, size_t object_size, char* output, size_t output_len) {
    unsigned char* bytes = (unsigned char*)object;
    unsigned long hash = 5381; // Initialize with a prime number (DJB2's initial value)

    // Process each byte
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; i < output_len; i++) {
        char byte = bytes[i % object_size];
        hash = (hash << 5) + hash + byte + 1;
        int index = hash % 36;
        output[i] = charset[index];
    }

    // null terminate
    output[output_len-1] = '\0';
}

static size_t hash(void* key, size_t key_size) {
    unsigned char* bytes = (unsigned char*)key;
    size_t hash = 5381;

    for (size_t i = 0; i < key_size; i++)
        hash = ((hash << 5) + hash) + bytes[i];

    return (size_t)hash;
}

static size_t hash3(int failures) {
    size_t hash = 5381;

    for (size_t i = 0; i < failures; i++)
        hash = (hash * 131) + failures;

    // not odd or zero
    if (hash % 2 == 0) {
        hash += 1;
    }

    return (size_t)hash;
}

static size_t probe(Map* map, void* key, size_t key_size, int* hash_collisions) {
    size_t index = hash(key, key_size) % map->data_size;
    size_t first_index = index;

    int open_or_match = map->data[index] == NULL;
    if (map->data[index] != NULL && map->data[index]->key_size == key_size) {
        open_or_match = open_or_match || memcmp(map->data[index]->key, key, key_size) == 0;
    }
    while (!open_or_match) {
        ++(*hash_collisions);
        index = (first_index + hash3(*hash_collisions)) % map->data_size;

        open_or_match = map->data[index] == NULL;
        if (map->data[index] != NULL && map->data[index]->key_size == key_size) {
            open_or_match = open_or_match || memcmp(map->data[index]->key, key, key_size) == 0;
        }
    }

    return index;
}


static void free_map_data(Map* map, int is_freeing_objects) {
    for (size_t i = 0; i < map->data_size; ++i) {
        if (map->data[i] != NULL) {
            free( map->data[i]->key);
            if (is_freeing_objects) {
                free(map->data[i]->data);
            }
            free( map->data[i]);
        }
    }
    free(map->data);
}

/*
    Used for freeing the maps meta data, freeing stored objects if specified.
*/
void free_map(Map* map, int is_freeing_objects) {
    free_map_data(map, is_freeing_objects);
    free(map);
}


static void insert_no_resize(Map* map, void* key, size_t key_size, void* data, size_t data_size) {
    int hash_collisions = 0;
    size_t index = probe(map, key, key_size, &hash_collisions);


    // if new element
    if (map->data[index] == NULL) {

        // copy the key
        char* key_copy = malloc(key_size);
        if (key_copy == NULL) {
            map_mem_error_exit_failing();
        }
        memcpy(key_copy, key, key_size);

        // make a new element
        Element *element = malloc(sizeof(Element));
        if (element == NULL) {
            map_mem_error_exit_failing();
        }
        element->key = key_copy;
        element->key_size = key_size;
        element->data = data;
        map->data[index] = element;

        ++map->len;
    }
    else {
        if (data_size != -1) {
            memcpy(map->data[index]->data, data, data_size);
        }
        else {
            fprintf(stderr, "Element already exists. Aborting to avoid leaving an unfreed pointer. (use 'm_put()' to overwrite elements or simply retrieve and modify elements) Exiting...");
            exit(EXIT_FAILURE);
        }

    }


}

static void resize_map(Map* map) {
    size_t NUM_PRIMES = sizeof(PRIMES) / sizeof(PRIMES[0]);

    // get next table size
    size_t new_table_size = PRIMES[0];
    for (int i = 0; i < NUM_PRIMES; ++i) {
        if (PRIMES[i] > map->data_size) {
            new_table_size = PRIMES[i];
            break;
        }
    }

    // make new table and insert each element
    size_t DELETED_KEY_SIZE = strlen(DELETED_KEY) + 1;
    Map* new_map = new_map_s(new_table_size);
    for (size_t i = 0; i < map->data_size; ++i) {

        // if an element insert into new map
        if (map->data[i] != NULL) {

            // check if deleted
            int deleted = 0;
            if (map->data[i]->key_size == DELETED_KEY_SIZE) {
                if (strcmp(map->data[i]->key, DELETED_KEY) == 0) {
                    deleted = 1;
                }
            }

            // an element insert into map
            if (!deleted) {
                Element* element = map->data[i];
                insert_no_resize(new_map, element->key, element->key_size, element->data, -1);
            }
        }
    }

    free_map_data(map, 0);
    map->data = new_map->data;
    map->data_size = new_map->data_size;
    map->len = new_map->len;

    new_map->data = NULL; // so it won't free the data array copied above
    free(new_map);
}


/*
    Function to insert an object in the map, with any other object used as the key.

    This can be used like so:
    ```
    Map* map = new_map();
    MyStruct key = {1, "hi"};
    MyStruct2 object = {1, 3};

    m_any_unique(map, &key, sizeof(key), &object);
    ```
    Does not allow overwriting existing elements, as this would leave an
    unfreed pointer. 

    Use the insert methods to overwrite object, or simply retrieve objects
    and modify them.

*/
void m_any_unique(Map* map, void* key, size_t key_size, void* data) {

    insert_no_resize(map, key, key_size, data, -1);

    // resize if neeeded
    if (map->data_size * 0.7 < map->len) {
        resize_map(map);
    }
}

/*
    Function to insert an object in the map using an int as the key.
    Does not allow overwriting existing elements, as this would leave an
    unfreed pointer. 

    Use the insert methods to overwrite object, or simply retrieve objects
    and modify them.
*/
void m_int_unique(Map* map, int key, void* data) {
    m_any_unique(map, &key, sizeof(int), data);
}

/*
    Function to insert an object in the map using a string as they key.
    Does not allow overwriting existing elements, as this would leave an
    unfreed pointer. 

    Use the insert methods to overwrite object, or simply retrieve objects
    and modify them.
*/
void m_unique(Map* map, char* key, void* data) {
    m_any_unique(map, key, (strlen(key) + 1) * sizeof(char), data);
}


/*
    Function to insert an object in the map, with any other object used as the key.

    This can be used like so:
    ```
    Map* map = new_map();
    MyStruct key = {1, "hi"};
    MyStruct2 object = {1, 3};

    m_any_put(map, &key, sizeof(key), &object, sizeof(object));
    ```

*/
void m_any_put(Map* map, void* key, size_t key_size, void* data, size_t data_size) {

    insert_no_resize(map, key, key_size, data, data_size);

    // resize if neeeded
    if (map->data_size * 0.7 < map->len) {
        resize_map(map);
    }
}

/*
    Function to insert an object in the map using an int as the key.
*/
void m_int_put(Map* map, int key, void* data, size_t data_size) {
    m_any_put(map, &key, sizeof(int), data, data_size);
}

/*
    Function to insert an object in the map using a string as the key.

    The key is copied so the key passed in can be freed before the map
    if needed. (the key copy is freed by the free_map() function)
*/
void m_put(Map* map, char* key, void* data, size_t data_size) {
    m_any_put(map, key, (strlen(key) + 1) * sizeof(char), data, data_size);
}


/*
    Function to get an object in the map, with any other object used as the key.

    This can be used like so:
    ```
    Map* map = new_map();
    MyStruct key = {1, "hi"};
    MyStruct2 object = (MyStruct2*) m_any_get((map, &key, sizeof(key));
    ```

    Returns a NULL pointer if no object exists at the key
*/
void* m_any_get(Map* map, void* key, size_t key_size) {
    int hash_collisions = 0;
    size_t index = probe(map, key, key_size, &hash_collisions);
    if(map->data[index] == NULL) {
        return NULL;
    }

    return map->data[index]->data;
}

/*
    Function to get an object in the map, using an int as the key
    Returns a NULL pointer if no object exists at the key
*/
void* m_int_get(Map* map, int key) {
    return m_any_get(map, &key, sizeof(key));
}

/*
    Function to get an object in the map, using an string as the key
    Returns a NULL pointer if no object exists at the key
*/
void* m_get(Map* map, char* key) {
    return m_any_get(map, key, (strlen(key) + 1) * sizeof(char));
}



/*
    Function to erase an object in the map, with any other object used as the key.

    This can be used like so:
    ```
    Map* map = new_map();
    MyStruct key = {1, "hi"};
    m_any_m_erase(map, &key, sizeof(key));
    ```

    If the key doesn't exist nothing happens
*/
void* m_any_m_erase(Map* map, void* key, size_t key_size) {
    int hash_collisions = 0;
    size_t index = probe(map, key, key_size, &hash_collisions);

    if (map->data[index] == NULL) {
        return NULL;
    }

    void* data = map->data[index]->data;
    Element* element = map->data[index];
    free(element->key);
    free(element);

    if (hash_collisions != 0) {

        // make deleted key
        size_t DELETED_KEY_SIZE = strlen(DELETED_KEY) + 1;
        char deleted_key_copy[DELETED_KEY_SIZE];
        strcpy(deleted_key_copy, DELETED_KEY);

        // set as deleted
        map->data[index]->key = deleted_key_copy;
        map->data[index]->key_size = DELETED_KEY_SIZE;
        map->data[index]->data = NULL;

        --map->len;
    }
    else {
        map->data[index] = NULL;
        --map->len;
    }

    return data;
}

/*
    Function to erase an object in the map, using an int as the key.

    If the key doesn't exist nothing happens
*/
void* m_int_m_erase(Map* map, int key) {
    return m_any_m_erase(map, &key, sizeof(key));
}

/*
    Function to erase an object in the map, using an string as the key.

    If the key doesn't exist nothing happens
*/
void* m_erase(Map* map, char* key) {
    return m_any_m_erase(map, key, (strlen(key) + 1) * sizeof(char));
}


/*
    Returns the elements in the map which are defined which
    contain both a key and a data array. Useful for iterating
    over map elements.

    These elements are only references to elements in the map.
    They should not be deleted or the map will be in a broken
    state. However, the array of elements must be cleaned up or
    there will be memory leaks. (so free the array, not the elements)
*/
Element** map_elements(Map* map) {
    Element** array = malloc(map->len * sizeof(Element));

    int l = 0;
    size_t DELETED_KEY_SIZE = strlen(DELETED_KEY) + 1;
    for (size_t i = 0; i < map->data_size; ++i) {
        if (map->data[i] != NULL) {
            
            int deleted = 0;
            if (map->data[i]->key_size == DELETED_KEY_SIZE) {
                if (strcmp(map->data[i]->key, DELETED_KEY) == 0) {
                    deleted = 1;
                }
            }

            if(!deleted) {
                array[l] = map->data[i];
                ++l;
            }
        }
    }

    return array;
}


/**
 * Clears the map, freeing stored objects if specified.
 */
void clear_map(Map* map, int is_freeing_objects) {
    free_map(map, is_freeing_objects);
    map = new_map();
}


/*
    Function to determine if an object is in the map, with any other object used as the key.

    This can be used like so:
    ```
    Map* map = new_map();
    MyStruct key = {1, "hi"};
    if(m_any_contains(map, &key, sizeof(key))) {
        printf("yay!");
    }
    ```
*/
int m_any_contains(Map* map, void* key, size_t key_size) {

    int hash_collisions = 0;
    size_t index = probe(map, key, key_size, &hash_collisions);
    if (map->data[index] == NULL) {
        return 0; 
    }

    return 1;
}

/*
    Function to determine if an object is in the map, with an int used as the key.
*/
int m_int_contains(Map* map, int key) {
    return m_any_contains(map, &key, sizeof(key));
}

/*
    Function to determine if an object is in the map, with an string used as the key.
*/
int m_contains(Map* map, char* key) {
    return m_any_contains(map, key, (strlen(key) + 1) * sizeof(char));
}

#endif

#ifndef LIST
#define LIST

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*
    A list of elements of any type with the following operation complexities
    - len(...) -> O(1)
    - l_push(...) -> O(1) amoritized
    - l_push_front(...) -> O(1) amoritized
    - l_pop(...) -> O(1)
    - l_pop_front(...) -> O(1)
    - l_get(...) -> O(1)
    - l_set(...) -> O(1)
    - l_clear() -> O(n)
    - free_list() -> O(n)
    - l_slice() -> O(n)


    This list implementation stores pointers to objects inserted by the user. These
    object can be on the stack or the heap. Object will NOT be cleaned up with
    free_list(), only the list's meta data is freed. 

    When done with your list you must call free_list() to avoid memory leaks.

    To update an index in the list do something like this:
    ```
    *(int*) l_get(list, 0) = 14;
    ```

    or this
    ```
    int data = 12;
    l_set(list, 0, &data, sizeof(int));
    ```


    Take care inserting objects on the stack. If those objects go out of scope or the objects
    are overwritten in a loop or something, the list will behave unexpectedly. 

    For example this
    ```

    List* list = new_list();
    for (int i = 0; i < 10; i++) {
        l_push(list, &i);
    }
    for(int i = 0; i < list->len; ++i) {
        int ele = *(int*) l_get(list, i);
        printf("%d, ", ele);
    }
    free_list(list);
    ```

    outputs:
    ```
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 
    ```

    To insert one through 9 you'd have to do this:
    ```

    List* list = new_list();
    for (int i = 0; i < 10; i++) {
        int* d = malloc(sizeof(int));
        *d = i;
        l_push(list, &d);
    }
    for(int i = 0; i < list->len; ++i) {
        int* ele = (int*) l_get(list, i);
        printf("%d, ", *ele);
        free(ele);
    }
    free_list(list);
    ```

*/
typedef struct List {
    void** data; // pointer to array of pointers
    size_t data_size;

    size_t start;
    size_t end;
    size_t len;
} List;



static void list_mem_error_exit_failing() {
    fprintf(stderr, "List couldn't get more memory on the system! Exiting...");
    exit(EXIT_FAILURE);
}


List* new_list_s(size_t size) {
    List *p_list = malloc(sizeof(List));
    if (p_list == NULL) {
        list_mem_error_exit_failing();
    }
    p_list->data = malloc(size * sizeof(void*));
    if (p_list->data == NULL) {
        free(p_list);
        list_mem_error_exit_failing();
    }
    p_list->data_size = size;
    p_list->start = 0;
    p_list->end = 0;
    p_list->len = 0;

    return p_list;
}

List* new_list() {
    return new_list_s(10);
}



/*
    doubles the internal array of the list

    returns 0 if successful
*/
int resize(List* list) {

    /*
        int size = size();
        T[] newData = (T[]) new Object[data.length * 2];
        if (startIndex > endIndex) {
            int startSize = this.data.length - startIndex;
            System.arraycopy(this.data, startIndex, newData, 0, startSize);
            System.arraycopy(this.data, 0, newData, startSize, endIndex);
        }
        else if (startIndex == endIndex) {
            // do nothing empty array
        }
        else {
            System.arraycopy(this.data, startIndex, newData, 0, size);
        }

        data = newData;
        startIndex = 0;
        endIndex = size;
    */

    size_t new_size = list->data_size * 2;
    void** new_data = malloc(new_size * sizeof(void*));
    if (new_data == NULL) {
        list_mem_error_exit_failing();
    }

    if (list->start > list->end) {

        // copy start index to array end
        memcpy(
            new_data, 
            list->data + list->start, 
            (list->data_size - list->start) * sizeof(void*)
        );
        // copy array start to end index
        memcpy(
            new_data + (list->data_size - list->start), 
            list->data, 
            list->end * sizeof(void*)
        );
    }
    else {
        memcpy(
            new_data, 
            list->data + list->start, 
            (list->end - list->start) * sizeof(void*)
        );
    }

    // freeing list but not contents
    free(list->data);
    list->data = new_data;
    list->data_size = new_size;
    list->start = 0;
    list->end = list->len;


    return 0;
}




/*
    Function to s_add to the end of a list.
    May resize interanl size of the list,
    but amoritized is a constant operation.

    A reference to either a stack or heap variable
    can be passed in. This pointer will be stored 
    in the list. If this data is freed or goes out
    of scope, the list will be in a bad state.
*/
void l_push(List* list, void* data) {
    
    if (list->len == list->data_size - 1) {
        resize(list);
    }

    list->data[list->end] = data;
    list->end = (list->end + 1) % list->data_size;
    list->len = list->len + 1;
}

/*
    Function to s_add to the end of a list.
    May resize interanl size of the list,
    but amoritized is a constant operation.

    A reference to either a stack or heap variable
    can be passed in. This pointer will be stored 
    in the list. If this data is freed or goes out
    of scope, the list will be in a bad state.
*/
void l_push_front(List* list, void* data) {

    if (list->len == list->data_size - 1) {
        resize(list);
    }

    list->start = (list->start - 1 + list->data_size) % list->data_size;
    list->data[list->start] = data;
    list->len = list->len + 1;
}

/*
    Removes and retrieves the last element of the list.

    The data is returned in the form void* and should probably
    be cast to the correct type:
    ```
    List* list = new_list();
    int i = 10;
    l_push(list, &i);
    int ele = *(int*) l_pop(list);
    ```
*/
void* l_pop(List* list) {

    if (list->len == 0) {
        return NULL;
    }


    list->end = (list->end - 1 + list->data_size) % list->data_size;
    list->len = list->len - 1;
    void* data = list->data[list->end];
    list->data[list->end] = NULL;

    return data;
}

/*
    Removes and retrieves the first element of the list.

    The data is returned in the form void* and should probably
    be cast to the correct type:
    ```
    List* list = new_list();
    int i = 10;
    l_push(list, &i);
    int ele = *(int*) l_pop(list);
    ```
*/
void* l_pop_front(List* list) {

    if (list->len == 0) {
        return NULL;
    }

    void* data = list->data[list->start];
    list->data[list->start] = NULL;
    list->start = (list->start + 1) % list->data_size;
    list->len = list->len - 1;

    return data;
}



static int convert_index(List* list, int index) {


    while (index < 0) {
        index += list->len;
    }

    int in_bounds = index < list->len;
    if (!in_bounds) {
        fprintf(stderr, "Index ");
        fprintf(stderr, "%d", index);
        fprintf(stderr, " out of bounds for len ");
        fprintf(stderr, "%d", list->len);
        exit(EXIT_FAILURE);
    }
    
    return (list->start + index) % list->data_size;
}


/*
    Get's a pointer to a list element (so changes to pointer object
    will change element on the list)
*/
void* l_get(List* list, int index) {
    int real_index = convert_index(list, index);
    return list->data[real_index];
}

/*
    Replaces the pointer stored at the given index with 'data'.
    No new memory is allocated; the pointer at that index is simply updated.
*/
void l_set(List* list, int index, void* data) {
    int real_index = convert_index(list, index);
    list->data[real_index] = data;
}



/*
    Clears the list, and frees all objects on the heap if specified.
*/
void l_clear(List* list, int is_freeing_objects) {
    // Free each pointer inside data
    if (is_freeing_objects) {
        for (size_t i = 0; i < list->len; i++) {
            free(l_get(list, i));
        }
    }
    list->len = 0;
    list->start = 0;
    list->end = 0;
}

/*
    frees the list and stored objects if specified.
*/
void free_list(List* list, int is_freeing_objects) {
    // Free data
    l_clear(list, is_freeing_objects);
    free(list->data);
    // Free the struct itself
    free(list);
}


/*
    Makes a new list from the specified range (inclusive exclusive).
    The resulting list will share objects with this list. (All objects
    in the list are just pointers to your objects)
*/
List* l_slice(List* list, int start, int end) {
    int real_start = convert_index(list, start);
    int real_end = convert_index(list, end);
    int new_len = 0;
    if (real_start > real_end) {
        new_len = list->data_size - real_start + real_end;
    }
    else {
        new_len = real_end - real_start;
    }

    List* new_list = new_list_s(new_len * 2);

    if (real_start > real_end) {

        // copy start index to array end
        memcpy(
            new_list->data, 
            list->data + real_start, 
            (list->data_size - real_start) * sizeof(void*)
        );
        // copy array start to end index
        memcpy(
            new_list->data + (list->data_size - real_start), 
            list->data, 
            real_end * sizeof(void*)
        );
    }
    else {
        memcpy(
            new_list->data, 
            list->data + real_start, 
            (real_end - real_start) * sizeof(void*)
        );
    }


    new_list->start = 0;
    new_list->end = new_len;
    new_list->len = new_len;

    return new_list;
}


void l_sort(List* list, int (* _Nonnull __compar)(const void *, const void *)) {
    resize(list); // get list data all in a row
    qsort(list->data, list->len, sizeof(void*),  __compar);
}



#endif

#ifndef WINDOWS_INIT
#define WINDOWS_INIT


    void wininit_init() {
        #ifdef _WIN32
        // INIT WINSOCK
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (result != 0) throw runtime_error("WSAStartup failed: " + to_string(result));
        #endif
    }

    void wininit_cleanup() {
        #ifdef _WIN32
        WSACleanup();
        #endif
    }

#endif

#ifndef TCP_SOCKET
#define TCP_SOCKET


    #include <fcntl.h>
    #include <sys/select.h>

    // Windows
    #ifdef _WIN32
    #pragma comment(lib,"ws2_32.lib")
    #define WIN32_LEAN_AND_MEAN
    #undef TEXT
    #include <winsock2.h>
    #include <ws2tcpip.h>

    // Linux
    #else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <errno.h>
    #ifndef SOCKET
    typedef int SOCKET;
    #endif
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #endif


    typedef struct {
        SOCKET socket_num;
        uint32_t timeout_ms;
    } TCPSocket;


    /**
     * Creates a TCP socket with the given timeout. If timeout_ms is -1, 
     * a default will be used.
     */
    TCPSocket* tcp_make_socket(uint32_t timeout_ms) {
        TCPSocket* n_socket = malloc(sizeof(TCPSocket));
        if (!n_socket) {
            fprintf(stderr, "Out of memory\n");
            return NULL;
        }

        n_socket->timeout_ms = timeout_ms == -1? 5000 : timeout_ms;
        n_socket->socket_num = socket(AF_INET, SOCK_STREAM, 0);
        if (n_socket->socket_num == INVALID_SOCKET) {
            printf(stderr, "Failed trying to create socket");
            return NULL;
        }

        // set timeout
        #ifdef _WIN32
        setsockopt(n_socket->socket_num, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
        #else
        // POSIX-specific socket setup (Linux, macOS)
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(n_socket->socket_num, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        #endif

        return n_socket;
    }


    /**
     * Cleans up a TCP socket and frees it
     */
    void tcp_clean_up_socket(TCPSocket* socket) { 

        #ifdef _WIN32
        closesocket(socket->socket_num);
        #else
        close(socket->socket_num);
        #endif

        free(socket);
    }


    int tcp_server_init(TCPSocket* socket, int port) {

        // Prepare the sockaddr_in structure
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(port);
        
        // Bind
        // Retry binding until it succeeds
        while (1) {
            if (bind(socket->socket_num, (struct sockaddr *)&server, sizeof(server)) == 0) {
                printf("Listening on port %d\n", port);
                break; // Bind succeeded
            }
            printf("Port %d in use or being cleaned up. Retrying...\n", port);
            sleep(5); // Wait for 5 seconds before retrying
        }

        // Listen to incoming connections
        if (listen(socket->socket_num, SOMAXCONN) < 0) {
            fprintf(stderr, "Listen failed\n");
            return -1;
        }

        // Set socket to non-blocking
        int flags = fcntl(socket->socket_num, F_GETFL, 0);
        fcntl(socket->socket_num, F_SETFL, flags | O_NONBLOCK);

        return 0;
    }


    SOCKET tcp_accept_connection(TCPSocket* socket, struct sockaddr_in *client_addr) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(socket->socket_num, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 5;  // 5 seconds timeout
        timeout.tv_usec = 0;

        int activity = select(socket->socket_num + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && errno != EINTR) return -1;

        if (activity == 0) return -1;

        socklen_t client_len = sizeof(client_addr);
        SOCKET client_socket = -1;
        if (FD_ISSET(socket->socket_num, &readfds)) {
            client_socket = accept(socket->socket_num, (struct sockaddr *)&client_addr, &client_len);
        }

        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "Failed to accept client connection\n");
            return -1;
        }

        return client_socket;
    }


    void tcp_connect(TCPSocket* socket, const char* serverIP, int port) {
        

        // set ip address
        #ifdef _WIN32
        struct sockaddr_in server;
        int slen2 = sizeof(server);
        ZeroMemory(&server, slen2);
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        #else
        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        #endif
        
        if (inet_pton(AF_INET, serverIP, &server.sin_addr) <= 0) {
            fprintf(stderr, "Invalid address/ Address not supported.\n");
            return;
        }

        // Connect to remote server
        if (connect(socket->socket_num, (struct sockaddr *)&server, sizeof(server)) != 0) {
            fprintf(stderr, "Connect error\n");
            return;
        }
    }

    int tcp_send(SOCKET client_socket, const char *message, int message_length) {
        #ifdef _WIN32
        u_long mode = 0; // 0 for blocking mode
        ioctlsocket(client_socket, FIONBIO, &mode);
        #else
        int flags = fcntl(client_socket, F_GETFL, 0);
        if (flags & O_NONBLOCK) {
            fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);
        }
        #endif
        
        int bytes_sent = send(client_socket, message, message_length, 0);
        if (bytes_sent == SOCKET_ERROR) {
            fprintf(stderr, "Failed to send message\n");
            return -1;
        }
        return bytes_sent;
    }

    int tcp_receive(TCPSocket* socket, SOCKET client_socket, char *buffer, int buffer_length) {
        // Ensure the socket is in blocking mode
        #if _WIN32
        u_long mode = 0; // 0 for blocking mode
        if (ioctlsocket(client_socket, FIONBIO, &mode) != 0) {
            fprintf(stderr, "Failed to set socket to blocking mode\n");
            return -1;
        }
        #else
        int flags = fcntl(client_socket, F_GETFL, 0);
        if (fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK) != 0) {
            fprintf(stderr, "Failed to set socket to blocking mode\n");
            return -1;
        }
        #endif

        // Set the receive timeout
        struct timeval timeout;
        timeout.tv_sec = socket->timeout_ms / 1000.0;
        timeout.tv_usec = (socket->timeout_ms % 1000) * 1000;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

        
        int bytes_received = recv(client_socket, buffer, buffer_length, 0);
        return bytes_received;
    }

    void tcp_close_connection(SOCKET client_socket) {
        #ifdef _WIN32
        closesocket(client_socket);
        #else
        close(client_socket);
        #endif
    }


#endif


#ifndef UDP_SOCKET
#define UDP_SOCKET

    // Windows
    #ifdef _WIN32
    #pragma comment(lib,"ws2_32.lib")
    #define WIN32_LEAN_AND_MEAN
    #undef TEXT
    #include <winsock2.h>
    #include <ws2tcpip.h>

    // Linux
    #else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #ifndef SOCKET
    typedef int SOCKET;
    #endif
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #endif


    typedef struct {
        SOCKET socket_num;
        uint32_t timeout_ms;
    } UDPSocket;


    /**
     * Creates a TCP socket with the given timeout. If timeout_ms is -1, 
     * a default will be used. Same for port
     */
    UDPSocket* udp_make_socket(UDPSocket* sock, uint32_t timeout_ms, int32_t port) {
        
        timeout_ms = timeout_ms == -1? 5000 : timeout_ms; 
        port = port == -1? 8080 : port; 


        #ifdef _WIN32
        // SET UP SOCKET
        struct addrinfo hints, *res;

        // set up address
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET; // Use IPv4
        hints.ai_socktype = SOCK_DGRAM; // UDP socket
        hints.ai_flags = AI_PASSIVE; // Use my IP

        // Get address info
        if (getaddrinfo(NULL, to_string(port).c_str(), &hints, &res) != 0) {
            fprintf(stderr, "Failed calling getaddrinfo trying to setup socket\n");
            return NULL;
        }

        // Create a socket
        sock->socket_num = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock->socket_num == INVALID_SOCKET) {
            fprintf(stderr, "Failed trying to create socket\n");
            return NULL;
        }

        // Bind socket
        if (bind(sock->socket_num, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
            fprintf(stderr, "Failed trying to bind socket\n");
            return NULL;
        }

        freeaddrinfo(res);
        
        // set timeout
        setsockopt(sock->socket_num, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout_ms, sizeof(timeout_ms));
        #else
        // POSIX-specific socket setup (Linux, macOS)
        sock->socket_num = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock->socket_num == INVALID_SOCKET) {
            fprintf(stderr, "Failed trying to create socket\n");
            return NULL;
        }
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sock->socket_num, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
            fprintf(stderr, "Failed trying to bind socket\n");
            return NULL;
        }

        // set timeout
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(sock->socket_num, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        #endif
    }

    /**
     * Cleans up a UDP socket and frees it
     */
    void udp_clean_up_socket(UDPSocket* sock) {
        #ifdef _WIN32
        closesocket(sock->socket_num);
        #else
        close(sock->socket_num);
        #endif

        free(sock);
    }


    /**
     * Blocks until it recieves a message. 
     * 
     * Returns gathered client info and stores message in buffer
     */
    struct sockaddr_in recieve(UDPSocket* sock, char *buffer, int read_size, int* bytes_read) {
            
        // make an empty client address
        struct sockaddr_in si_client;
        socklen_t slen = sizeof(si_client);

        // recieve message
        *bytes_read = recvfrom(sock->socket_num, buffer, read_size, 0, (struct sockaddr *) &si_client, &slen);

        return si_client;
    };
    

    /**
     * Sends a message to a host
     */
    int send_udp(UDPSocket* sock, const char *message, const int length, const char* host, int port) {

        #ifdef _WIN32

        // set up address
        struct sockaddr_in si_server;
        int slen2 = sizeof(si_server);
        ZeroMemory(&si_server, slen2);
        si_server.sin_family = AF_INET;
        si_server.sin_port = htons(port);

        // set binary IP address in the address object
        inet_pton(AF_INET, host, &si_server.sin_addr);

        // send message
        int bytes_sent = sendto(sock->socket_num, message, length, 0, (struct sockaddr *) &si_server, slen2);
        if (bytes_sent == SOCKET_ERROR) {
            fprintf(stderr, "Bytes failed to send\n");
            return -1;
        }
        
        return bytes_sent;

        #else

        // set up address
        struct sockaddr_in si_server;
        socklen_t slen2 = sizeof(si_server);
        memset(&si_server, 0, slen2); // Use memset instead of ZeroMemory
        si_server.sin_family = AF_INET;
        si_server.sin_port = htons(port);

        // set binary IP address in the address object
        if (inet_pton(AF_INET, host, &si_server.sin_addr) <= 0) {
            fprintf(stderr, "Invalid address/ Address not supported\n");
            return -1;
        }

        // send message
        int bytes_sent = sendto(sock->socket_num, message, length, 0, (struct sockaddr *) &si_server, slen2);
        if (bytes_sent == -1) {
            int err = errno;
            fprintf(stderr, "Bytes failed to send, error code: %d ( %s )\n", err, strerror(err));
            return -1;
        }

        return bytes_sent;

        #endif
    };

#endif



#ifndef C_SERVER
#define C_SERVER


    #include <pthread.h>
    #include <time.h>

    // ----------------------------
    // Colors
    // ----------------------------
    #define RED      "\x1b[38;2;255;0;0m"
    #define ORANGE   "\x1b[38;2;230;76;0m"
    #define YELLOW   "\x1b[38;2;230;226;0m"
    #define GREEN    "\x1b[38;2;0;186;40m"
    #define BLUE     "\x1b[38;2;0;72;255m"
    #define INDIGO   "\x1b[38;2;84;0;230m"
    #define VIOLET   "\x1b[38;2;176;0;230m"
    #define GREY   "\x1b[38;2;105;105;105m"
    #define ANSI_RESET "\x1b[0m"


    #define TRACE "TRACE"
    #define DEBUG "DEBUG"
    #define INFO "INFO"
    #define WARN "WARN"
    #define ERROR "ERROR"


    char* log_file_path = NULL;


    void log(char* message, char* level) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

        char* color;
        if (strcmp(level, TRACE)) color = YELLOW;
        else if (strcmp(level, DEBUG)) color = GREEN;
        else if (strcmp(level, INFO)) color = BLUE;
        else if (strcmp(level, WARN)) color = ORANGE;
        else if (strcmp(level, ERROR)) color = RED;

        size_t size = strlen(color) + strlen(level) + strlen(time_str) + strlen(message) + 128;

        char *entire_message = malloc(size);
        if (!entire_message)
            return;

        /* Build formatted message */
        printf(
            "%s[%s]%s %s%s%s %s\n",
            color,
            level,
            ANSI_RESET,
            GREY,
            time_str,
            ANSI_RESET,
            message
        );

        /* Write to file (without ANSI colors) */
        FILE *file = fopen("app.log", "a");
        if (file) {
            fprintf(file, "[%s] %s %s\n", level, time_str, message);
            fclose(file);
        }

        free(entire_message);


    }
    

    typedef enum {
         // 1xx Informational
        HTTP_CONTINUE = 100,
        HTTP_SWITCHING_PROTOCOLS = 101,
        HTTP_PROCESSING = 102,

        // 2xx Success
        HTTP_OK = 200,
        HTTP_CREATED = 201,
        HTTP_ACCEPTED = 202,
        HTTP_NON_AUTHORITATIVE_INFORMATION = 203,
        HTTP_NO_CONTENT = 204,
        HTTP_RESET_CONTENT = 205,
        HTTP_PARTIAL_CONTENT = 206,

        // 3xx Redirection
        HTTP_MULTIPLE_CHOICES = 300,
        HTTP_MOVED_PERMANENTLY = 301,
        HTTP_FOUND = 302,
        HTTP_SEE_OTHER = 303,
        HTTP_NOT_MODIFIED = 304,
        HTTP_USE_PROXY = 305,
        HTTP_TEMPORARY_REDIRECT = 307,
        HTTP_PERMANENT_REDIRECT = 308,

        // 4xx Client Errors
        HTTP_BAD_REQUEST = 400,
        HTTP_UNAUTHORIZED = 401,
        HTTP_PAYMENT_REQUIRED = 402,
        HTTP_FORBIDDEN = 403,
        HTTP_NOT_FOUND = 404,
        HTTP_METHOD_NOT_ALLOWED = 405,
        HTTP_NOT_ACCEPTABLE = 406,
        HTTP_REQUEST_TIMEOUT = 408,
        HTTP_CONFLICT = 409,
        HTTP_GONE = 410,
        HTTP_LENGTH_REQUIRED = 411,
        HTTP_PRECONDITION_FAILED = 412,
        HTTP_PAYLOAD_TOO_LARGE = 413,
        HTTP_URI_TOO_LONG = 414,
        HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
        HTTP_TOO_MANY_REQUESTS = 429,

        // 5xx Server Errors
        HTTP_INTERNAL_SERVER_ERROR = 500,
        HTTP_NOT_IMPLEMENTED = 501,
        HTTP_BAD_GATEWAY = 502,
        HTTP_SERVICE_UNAVAILABLE = 503,
        HTTP_GATEWAY_TIMEOUT = 504,
        HTTP_HTTP_VERSION_NOT_SUPPORTED = 505
    } HttpStatus;

    const char* http_status_reason(HttpStatus status) {
        switch (status) {

            /* 1xx Informational */
            case HTTP_CONTINUE:                        return "Continue";
            case HTTP_SWITCHING_PROTOCOLS:             return "Switching Protocols";
            case HTTP_PROCESSING:                      return "Processing";

            /* 2xx Success */
            case HTTP_OK:                              return "OK";
            case HTTP_CREATED:                        return "Created";
            case HTTP_ACCEPTED:                       return "Accepted";
            case HTTP_NON_AUTHORITATIVE_INFORMATION:  return "Non-Authoritative Information";
            case HTTP_NO_CONTENT:                     return "No Content";
            case HTTP_RESET_CONTENT:                  return "Reset Content";
            case HTTP_PARTIAL_CONTENT:                return "Partial Content";

            /* 3xx Redirection */
            case HTTP_MULTIPLE_CHOICES:                return "Multiple Choices";
            case HTTP_MOVED_PERMANENTLY:               return "Moved Permanently";
            case HTTP_FOUND:                           return "Found";
            case HTTP_SEE_OTHER:                       return "See Other";
            case HTTP_NOT_MODIFIED:                    return "Not Modified";
            case HTTP_USE_PROXY:                      return "Use Proxy";
            case HTTP_TEMPORARY_REDIRECT:              return "Temporary Redirect";
            case HTTP_PERMANENT_REDIRECT:              return "Permanent Redirect";

            /* 4xx Client Errors */
            case HTTP_BAD_REQUEST:                     return "Bad Request";
            case HTTP_UNAUTHORIZED:                    return "Unauthorized";
            case HTTP_PAYMENT_REQUIRED:                return "Payment Required";
            case HTTP_FORBIDDEN:                       return "Forbidden";
            case HTTP_NOT_FOUND:                       return "Not Found";
            case HTTP_METHOD_NOT_ALLOWED:              return "Method Not Allowed";
            case HTTP_NOT_ACCEPTABLE:                  return "Not Acceptable";
            case HTTP_REQUEST_TIMEOUT:                 return "Request Timeout";
            case HTTP_CONFLICT:                        return "Conflict";
            case HTTP_GONE:                            return "Gone";
            case HTTP_LENGTH_REQUIRED:                 return "Length Required";
            case HTTP_PRECONDITION_FAILED:             return "Precondition Failed";
            case HTTP_PAYLOAD_TOO_LARGE:               return "Payload Too Large";
            case HTTP_URI_TOO_LONG:                    return "URI Too Long";
            case HTTP_UNSUPPORTED_MEDIA_TYPE:          return "Unsupported Media Type";
            case HTTP_TOO_MANY_REQUESTS:               return "Too Many Requests";

            /* 5xx Server Errors */
            case HTTP_INTERNAL_SERVER_ERROR:            return "Internal Server Error";
            case HTTP_NOT_IMPLEMENTED:                  return "Not Implemented";
            case HTTP_BAD_GATEWAY:                      return "Bad Gateway";
            case HTTP_SERVICE_UNAVAILABLE:              return "Service Unavailable";
            case HTTP_GATEWAY_TIMEOUT:                  return "Gateway Timeout";
            case HTTP_HTTP_VERSION_NOT_SUPPORTED:       return "HTTP Version Not Supported";

            default:
                return "Unknown Status";
        }
    }
    
    typedef struct {
        void* data;  
        int thread_id;
        const char *name;
    } ThreadWorkerData;

    typedef struct {
        char* key;
        char* value;
    } HttpHeader;

    typedef struct {
        struct sockaddr_in client;
        SOCKET socket;
    } OpenSocket;

    typedef struct {
        HttpHeader* headers;
        int num_headers;
        char* body;
        int content_length;
    } Request;

    typedef struct {
        HttpStatus status;
        char* message;
        HttpHeader* headers;
        int num_headers;
        char* body;
        int content_length;
    } Response;


    typedef struct {
        Map* ip_address_to_requests_in_window;
        int rate_limit_window_s;
        int requests_per_window;
        uint64_t last_reset_s;

    } DefaultSecurityData;


    typedef struct CServer CServer;
    typedef struct CServer {

        // sockets
        TCPSocket* tcp_socket;
        UDPSocket* udp_socket;

        // server setting
        int port;
        int timeout_ms; 
        int max_request_size_mb; 
        int num_threads;
        Response (*handler_function)(char* method, HttpHeader* headers, int num_headers, char* path, char* body);

        // thread handling
        pthread_t *socket_thread;
        pthread_t *threads;
        pthread_mutex_t lock;
        pthread_cond_t notify;
        int shutdown;
        List* requests;

        // security 
        pthread_mutex_t security_data_lock;
        void* security_data;
        int (*rate_limiter)(char* ip_address, int port, CServer* c_server);


    } CServer;


    // UTIL functions

    /**
     * Removes whitespace from the end of the string, returning the same string
     */
    static char *trim(char *s) {
        while (*s == ' ' || *s == '\t') s++;
        char *end = s + strlen(s) - 1;
        while (end > s && (*end == ' ' || *end == '\t')) *end-- = '\0';
        return s;
    }

    /**
     * Makes a new string on the heap with the two provided strings. Flags are used to determine if input strings are freed.
     */
    static char* str_append(char* str, char* addition, int free_str, int free_addition) {
        if (str == NULL || addition == NULL) {
            return str; // or handle error
        }

        size_t len_str = strlen(str);
        size_t len_add = strlen(addition);
        
        char* new_str = malloc(len_str + len_add + 1);


        memcpy(new_str, str, len_str);
        memcpy(new_str + len_str, addition, len_add+1); // +1 to copy null terminator

        return new_str;
    }


    typedef struct {
        void **data;
        size_t length;
        size_t capacity;
    } ptr_array;

    static int ptr_array_push(ptr_array *a, void *value) {
        if (a->length >= a->capacity) {
            size_t new_cap = a->capacity ? a->capacity * 2 : 8;
            void **tmp = realloc(a->data, new_cap * sizeof(void *));
            if (!tmp) return -1;
            a->data = tmp;
            a->capacity = new_cap;
        }

        a->data[a->length++] = value;
        return 0;
    }

    static int free_ptr_array(ptr_array *a, int free_values) {
        if (free_values) {
            for (int i = 0; i < a->length; i++) {
                free(a->data[i]);
            }
        }
        free(a);
    }

    typedef struct {
        char *data;
        size_t len;
        size_t cap;
    } strbuf;

    static int strbuf_init(strbuf *s, int intial_size) {
        s->data = malloc(intial_size);
        if (!s->data) return -1;
        s->len = 0;
        s->cap = intial_size;
    }

    static int strbuf_append(strbuf *s, const char *str) {
        size_t n = strlen(str);

        int needed = s->len + n + 1;
        if (needed > s->cap) {
            size_t new_cap = s->cap ? s->cap : 16;
            while (new_cap < needed)
                new_cap *= 2;
            char *tmp = realloc(s->data, new_cap);
            if (!tmp) return -1;
            s->data = tmp;
            s->cap = new_cap;
        }

        memcpy(s->data + s->len, str, n);
        s->len += n;
        s->data[s->len] = '\0';
        
        return 0;
    }

    static void free_headers(HttpHeader** headers, int num_headers) {
        for (int i = 0; i < num_headers; i++) {
            HttpHeader* header = headers[i];
            free(header->key);
            free(header->value);
            free(header);
        }
        free(headers);
    }



    // MAIN server functions
    void c_server_free(CServer* server) {
        if (server->tcp_socket != NULL)
            tcp_clean_up_socket(server->tcp_socket);
        if (server->udp_socket != NULL)
            udp_clean_up_socket(server->tcp_socket);

        free(server->socket_thread);
        for (int i = 0; i < server->num_threads; i++) {
            free(server->threads[i]);
        }
        free(server->threads);

        free_list(server->requests, 1);
    }

    static int defualt_rate_limiter(char* ip_address, int port, CServer* server) {
        DefaultSecurityData* sec_data = (Map*) server->security_data;
        
        // update counts
        int* count;
        if(m_contains(sec_data->ip_address_to_requests_in_window, ip_address)) {
            count = m_get(sec_data->ip_address_to_requests_in_window, ip_address);
            *count++;
            m_put(sec_data->ip_address_to_requests_in_window, ip_address, count, sizeof(int));
        }
        else {
            count = malloc(sizeof(int));
            *count = 1;
            m_put(sec_data->ip_address_to_requests_in_window, ip_address, count, sizeof(int));
        }
        
        // reset window when time
        time_t sec = time(NULL);
        if (sec - sec_data->last_reset_s > sec_data->rate_limit_window_s) {
            free_map(sec_data->ip_address_to_requests_in_window, 1);
            sec_data->ip_address_to_requests_in_window = new_map();
            sec_data->last_reset_s = sec;
        }
        
        // return error if ip has hit threshold
        if (*count > sec_data->requests_per_window) {
            return -1;
        }
        

        return 0;
    }

    static void *socket_thread_func(void *arg) {
        CServer *server = (CServer *)arg;
        server->tcp_socket = tcp_make_socket(server->timeout_ms);
        tcp_server_init(server->tcp_socket, server->port);
        
        int buffer_len = server->max_request_size_mb * 1000;
        char* buffer = malloc(sizeof(char) * buffer_len);
        while(!server->shutdown) {

            // accept connection
            OpenSocket* request = malloc(sizeof(OpenSocket));
            request->socket = tcp_accept_connection(server->tcp_socket, &request->client);
            if (request->socket == -1) continue;

            // submit to workers
            pthread_mutex_lock(&server->lock);
            printf("Main thread got lock\n");
            if (server->shutdown) {
                pthread_mutex_unlock(&server->lock);
                free(request);
                break;
            }
            l_push(server->requests, request);
            pthread_cond_signal(&server->notify);
            pthread_mutex_unlock(&server->lock);
        }

    }

    static void *worker_thread(void *arg) {
        ThreadWorkerData *data = (ThreadWorkerData *)arg;
        CServer *server = (CServer *)data->data;

        while (1) {

            // ACQUIRE lock and get request off queue
            pthread_mutex_lock(&server->lock);
            while (server->requests->len == 0 && !server->shutdown) {
                int res = pthread_cond_wait(&server->notify, &server->lock);
                // printf("%d\n", res);
            }
            if (server->shutdown) {
                pthread_mutex_unlock(&server->lock);
                log("-- Thread shutting down", DEBUG);
                pthread_exit(NULL);
            }
            OpenSocket* request = NULL;
            if (server->requests->len > 0) {
                request = (OpenSocket*) l_pop_front(server->requests);
            }
            pthread_mutex_unlock(&server->lock);
            if (request == NULL) continue;


            // get ip address and port
            char ip_address[INET6_ADDRSTRLEN];
            if (request->client.sin_family == AF_INET6) {
                inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&request->client)->sin6_addr, ip_address, sizeof(ip_address));
            } 
            else {
                inet_ntop(AF_INET, &((struct sockaddr_in *)&request->client)->sin_addr, ip_address, sizeof(ip_address));
            }
            int port = ntohs(request->client.sin_port);


            // RATE limit based on ip
            pthread_mutex_lock(&server->security_data_lock);
            printf("Thread %d lock rate limit\n", data->thread_id);
            int should_limit = server->rate_limiter(ip_address, port, server);
            printf("Thread %d unlock rate limit\n", data->thread_id);
            pthread_mutex_unlock(&server->security_data_lock);
            if (should_limit) {
                goto close_connection;
            }


            // READ message
            ptr_array headers; 
            headers.data = malloc(sizeof(HttpHeader) * 8);
            headers.capacity = 8;
            headers.length = 0;
            int num_headers = 0;
            char *path = malloc(sizeof(char)*1024);
            char *body = malloc(1);
            int buffer_len = server->max_request_size_mb * 1000;
            char* buffer = malloc(sizeof(char) * buffer_len);
            int bytes_recieved = tcp_receive(server->tcp_socket, request->socket, buffer, buffer_len);
            if (bytes_recieved == buffer_len) {
                char message[1024];
                snprintf(message, 1024, "Message exceeded max request size of %dmb", server->max_request_size_mb);
                log(message, ERROR);
                goto cleanup;
            }


            // PARSE REQUEST

            /* 1. Find header/body boundary */
            char *headers_end = strstr(buffer, "\r\n\r\n");
            if (!headers_end) continue;

            *headers_end = '\0';
            char *body_start = headers_end + 4;

            /* 2. Parse request line */
            char *line_end = strstr(buffer, "\r\n");
            if (!line_end) continue;
            *line_end = '\0';

            char method[8], version[16];
            sscanf(buffer, "%7s %1023s %15s", method, path, version);

            /* 3. Parse headers */
            int content_length = 0;
            char *line = line_end + 2;
            while (line && *line) {
                char *next = strstr(line, "\r\n");
                if (next) *next = '\0';

                char *colon = strchr(line, ':');
                if (colon) {
                    *colon = '\0';
                    char *key = trim(line);
                    char *value = trim(colon + 1);

                    if (strcmp(key, "Content-Length")) {
                        int len = atoi(value);
                    }
                    
                    HttpHeader* header = malloc(sizeof(HttpHeader));
                    header->key = strdup(key);
                    header->value = strdup(value);
                    ptr_array_push(&headers, header);
                    ++num_headers;
                }

                if (!next) break;
                line = next + 2;
            }

            /* 4. Parse body */
            if (!content_length) {
                int header_length = body_start - buffer;
                content_length = bytes_recieved - header_length;
            }
            if (content_length) {
                body = realloc(body, content_length + 1);
                memcpy(body, body_start, content_length);
                body[content_length] = '\0';
            }



            // CALL USER DEFINED HANDLER
            Response response = server->handler_function(method, headers.data, num_headers, path, body);
            
            
            // CONVERT RESPONSE TO HTTP RESPONSE
            strbuf buf;
            strbuf_init(&buf, response.content_length + 4096);

            /* Status line */
            char status_line[1024];
            snprintf(
                status_line,
                1024,
                "HTTP/1.1 %d %s\r\n",
                response.status,
                http_status_reason(response.status)
            );
            strbuf_append(&buf, status_line);


            /* Headers from map */
            for (int i = 0; i < response.num_headers; i++) {
                char* key = response.headers[i].key;
                char* value = response.headers[i].value;

                strbuf_append(&buf, key);
                strbuf_append(&buf, ": ");
                strbuf_append(&buf, value);
                strbuf_append(&buf, "\r\n");
            }

            // content length
            strbuf_append(&buf, "Content-Length: ");
            char c_length[32];
            snprintf(c_length, 32, "%d", response.content_length);
            strbuf_append(&buf, c_length);
            strbuf_append(&buf, "\r\n");

            strbuf_append(&buf, "\r\n");


            // add body
            strbuf_append(&buf, response.body);



            // SEND response
            printf("Response: \n\n%s\n", buf.data);
            tcp_send(request->socket, buf.data, buf.len);


            repsonse_cleanup:
                free(buf.data);
            
            cleanup:
                free_headers(headers.data, headers.length);
                free(path);
                free(body);
                free(buffer);

            close_connection:
                tcp_close_connection(request->socket);
                free(request);
        }
    }
    


    CServer* c_server_start_tcp_options(
        int port, 
        int timeout_ms, 
        int max_request_size_mb, 
        int threads, 
        Response (*handler_function)(char* method, HttpHeader* headers, int num_headers, char* path, char* body),
        int (*rate_limiter)(char* ip_address, int port, CServer* server),
        void* security_data
    ) {
        CServer* server = malloc(sizeof(CServer));
        server->udp_socket = NULL;
        server->port = port;
        server->timeout_ms = timeout_ms;
        server->max_request_size_mb = max_request_size_mb;
        server->num_threads = threads;
        server->security_data = security_data;
        server->rate_limiter = rate_limiter;
        server->handler_function = handler_function;
        server->shutdown = 0;
        server->requests = new_list();

        server->threads = malloc(sizeof(pthread_t) * threads);

        // initialize mutexes
        pthread_mutex_init(&server->lock, NULL);
        pthread_cond_init(&server->notify, NULL);
        pthread_mutex_init(&server->security_data_lock, NULL);


        // start socket
        pthread_create(&server->socket_thread, NULL, socket_thread_func, server);

        // start workers
        for (int i = 0; i < threads; i++) {
            ThreadWorkerData* data = malloc(sizeof(ThreadWorkerData));
            data->data = server;
            data->thread_id = i;
            pthread_create(&server->threads[i], NULL, worker_thread, data);
        }


        return server;
    }

    CServer* c_server_start_tcp(int port, Response (*handler_function)(char* method, HttpHeader* headers, int num_headers, char* path, char* body)) {

        DefaultSecurityData* sec_data = malloc(sizeof(DefaultSecurityData));
        sec_data->ip_address_to_requests_in_window = new_map();
        sec_data->rate_limit_window_s = 1 * 60 * 1000;
        sec_data->requests_per_window = 50;

        return c_server_start_tcp_options(
            port,
            30000,
            50,
            8,
            handler_function,
            defualt_rate_limiter,
            sec_data
        );
    }

    CServer* cweb_start_udp(int port, int timeout_ms, int max_request_size_mb, int threads, Response (*handler_function)(char* method, HttpHeader* headers, int num_headers, char* path, char* body)) {

    }


    void c_server_stop(CServer* server) {
        pthread_mutex_lock(&server->lock);
        server->shutdown = 1;
        pthread_cond_broadcast(&server->notify);
        pthread_mutex_unlock(&server->lock);

        for (int i = 0; i < server->num_threads; i++) {
            pthread_join(server->threads[i], NULL);
        }
        pthread_join(server->socket_thread, NULL);

        pthread_mutex_destroy(&server->lock);
        pthread_cond_destroy(&server->notify);
    }


#endif







