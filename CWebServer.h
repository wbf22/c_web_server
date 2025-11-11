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
            printf("Port %d in use or being cleaned up. Retrying...\n", port);
            if (bind(socket->socket_num, (struct sockaddr *)&server, sizeof(server)) == 0) {
                break; // Bind succeeded
            }
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
            return -1;
        }

        // Connect to remote server
        if (connect(socket->socket_num, (struct sockaddr *)&server, sizeof(server)) != 0) {
            fprintf(stderr, "Connect error\n");
            return -1;
        }

        return 0;
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
    struct sockaddr_in recieve(char *buffer, int read_size, int* bytes_read) {
            
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
    int send(const char *message, const int length, const char* host, int port) {

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



#ifndef C_WEB_SERVER
#define C_WEB_SERVER

    typedef struct {
        TCPSocket* tcp_socket;
        UDPSocket* udp_socket;
    } CWebServer;

    CWebServer* cweb_start(int port, int timeout_ms) {

    }

    CWebServer* cweb_start_udp(int port, int timeout_ms) {

    }

    void cweb_clean_up() {

    }

#endif







