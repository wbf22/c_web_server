#include <stdio.h>
#include "List.h"
#include "Map.h"
#include "String.h"
#include "Set.h"
#include "CsvDb.h"

/*

valgrind --leak-check=full --show-leak-kinds=all ./test_exe

leaks -atExit -- ./test_exe
*/

char* ANSII_RESET = "\033[0m";
void assert(int condition, char* message) {
    if (!condition) {
        fprintf(stderr, "\033[38;2;255;0;0m");
        fprintf(stderr, "failed: ");
        fprintf(stderr, message);
        fprintf(stderr, ANSII_RESET);
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    }
    else {
        fprintf(stderr, "\033[38;2;40;133;0m");
        fprintf(stderr, "passed: ");
        fprintf(stderr, message);
        fprintf(stderr, ANSII_RESET);
        fprintf(stderr, "\n");
    }
}


void* copy_data(void* data, size_t size) {
    void* ptr = malloc(size);
    // int c = *(int*) data;
    memcpy(ptr, data, size);
    return ptr;
}

void list_test_push_push_front_pop_pop_front_resize_get_set() {

    List* list = new_list();


    int i1 = 1;
    l_push(list, &i1);
    int i2 = 2;
    l_push(list, &i2);
    int i3 = 3;
    l_push(list, &i3);
    int i4 = 4;
    l_push(list, &i4);
    int i5 = 5;
    l_push(list, &i5);
    int i6 = 6;
    l_push(list, &i6);
    int i7 = 7;
    l_push(list, &i7);
    int i8 = 8;
    l_push(list, &i8);
    int i9 = 9;
    l_push(list, &i9);
    int i10 = 10;
    l_push(list, &i10);
    int i11 = 11;
    l_push(list, &i11);
    int i12 = 12;
    l_push(list, &i12);


    int b = 0;
    b = *(int*) l_pop(list);
    assert(12 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(11 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(10 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(9 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(8 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(7 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(6 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(5 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(4 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(3 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(2 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(1 == b, "push pop same");

    free_list(list, 0);




    list = new_list();
    i1 = 1;
    l_push_front(list, &i1);
    i2 = 2;
    l_push_front(list, &i2);
    i3 = 3;
    l_push(list, &i3);
    i4 = 4;
    l_push(list, &i4);
    i5 = 5;
    l_push(list, &i5);
    i6 = 6;
    l_push(list, &i6);
    i7 = 7;
    l_push(list, &i7);
    i8 = 8;
    l_push(list, &i8);
    i9 = 9;
    l_push(list, &i9);
    i10 = 10;
    l_push(list, &i10);
    i11 = 11;
    l_push(list, &i11);
    i12 = 12;
    l_push(list, &i12);


    b = 0;
    b = *(int*) l_pop_front(list);
    assert(2 == b, "push pop same");
    b = *(int*) l_pop_front(list);
    assert(1 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(12 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(11 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(10 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(9 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(8 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(7 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(6 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(5 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(4 == b, "push pop same");
    b = *(int*) l_pop(list);
    assert(3 == b, "push pop same");

    free_list(list, 0);
    



    list = new_list();

    i1 = 1;
    l_push_front(list, &i1);
    i2 = 2;
    l_push_front(list, &i2);
    i3 = 3;
    l_push(list, &i3);
    i4 = 4;
    l_push(list, &i4);
    i5 = 5;
    l_push(list, &i5);
    i6 = 6;
    l_push(list, &i6);
    i7 = 7;
    l_push(list, &i7);
    i8 = 8;
    l_push(list, &i8);
    i9 = 9;
    l_push(list, &i9);
    i10 = 10;
    l_push(list, &i10);
    i11 = 11;
    l_push(list, &i11);
    i12 = 12;
    l_push(list, &i12);


    // 2 1 3 4 5 6 7 8 9 10 11 12
    b = 0;
    b = *(int*) l_pop_front(list);
    b = *(int*) l_pop(list);
    b = *(int*) l_pop(list);

    // 1 3 4 5 6 7 8 9 10
    int i13 = 13;
    l_push(list, &i13);
    int i14 = 14;
    l_push(list, &i14);
    int i15 = 15;
    l_push(list, &i15);

    // 1 3 4 5 6 7 8 9 10 13 14 15
    int* t = (int*) l_get(list, 0);
    assert(*t == 1, "index get");
    t = (int*) l_get(list, 1);
    assert(*t == 3, "index get");
    t = (int*) l_get(list, -1);
    assert(*t == 15, "index get");
    for(int i = 0; i < list->len; ++i) {
        int ele = *(int*) l_get(list, i);
        printf("%d, ", ele);
    }
    printf("\n");

    List* slice_list = l_slice(list, 1, 4);
    for(int i = 0; i < slice_list->len; ++i) {
        int ele = *(int*) l_get(slice_list, i);
        printf("%d, ", ele);
    }
    printf("\n");
    t = (int*) l_get(slice_list, 0);
    assert(*t == 3, "slice check");
    t = (int*) l_get(slice_list, 1);
    assert(*t == 4, "slice check");
    t = (int*) l_get(slice_list, -1);
    assert(*t == 5, "slice check");
    free_list(slice_list, 0);


    free_list(list, 0);


}

void map_resizing_all_methods() {

    // Testing with Structs
    typedef struct MyStruct {
        int x;
        int y;
    } MyStruct;

    Map* map = new_map();
    MyStruct object = {1, 3};
    m_unique(map, "my_key", &object);

    MyStruct back_out = *(MyStruct*) m_get(map, "my_key");

    m_erase(map, "my_key");

    printf("%d\n", map->len);


    MyStruct object2 = {1, 3};
    m_unique(map, "my_key", &object2);
    MyStruct object3 = {2, 4};
    m_unique(map, "my_key2", &object3);

    Element** items = map_elements(map);
    for (int i = 0; i < map->len; ++i) {
        Element* item = items[i];
        char* key = item->key;
        MyStruct obj = *(MyStruct*) item->data;

        printf("%s{%d,%d}\n", key, obj.x, obj.y);
    }
    free(items);

    free_map(map, 0);



    // testing resize
    map = new_map();
    for (int i = 0; i < PRIMES[2]; ++i) {
        int *data = malloc(sizeof(int));
        *data = i * 2;
        m_int_unique(map, i, data);
    }

    for (int i = 0; i < PRIMES[2]; ++i) {
        int* data = (int*) m_int_m_erase(map, i);        
        assert(*data == i * 2, "check int map");

        free(data);
    }
    free_map(map, 0);



    // 
    Map* checker = new_map();

    char* key = "hi";
    size_t key_size = (strlen(key) + 1) * sizeof(char);
    size_t table_size = 57;
    size_t index = hash(key, key_size) % table_size;
    int* hash_collisions = malloc(sizeof(int));
    *hash_collisions = 0;
    for(int i = 0; i < table_size * 2; ++i) {
        ++(*hash_collisions);
        index = (index + hash3(*hash_collisions)) % table_size;

        int* count;
        if (m_int_contains(checker, index)) {
            void* ptr = m_int_get(checker, index);
            count = (int*) ptr;
            // free(ptr);
        }
        else {
            count = malloc(sizeof(int));
            *count = 0;
            m_int_unique(checker, index, count);
        }
        *count = *count + 1;

        // void* copy = copy_data(&count, sizeof(int));
        // m_int_put(checker, index, copy);

        int d = *(int*) m_int_get(checker, index);
        printf("%d\n", d);
    }
    free(hash_collisions);
    


    for(int i = 0; i < table_size; ++i) {
        void* ptr = m_int_m_erase(checker, i);
        int count = 0;
        if (ptr != NULL) {
            count = *(int*) ptr;
        }
        printf("%d %d\n", i, count);
        free(ptr);
    }
    free_map(checker, 0);

}

void stringstream_test() {

    String* ss = new_string();

    append(ss, "hi ");
    append(ss, "bye! ");
    append_front(ss, "what ");
    append_front(ss, "That's ");
    String* sub = substr(ss, 4, 14);
    printf("%s\n", str(sub));
    free_string(sub);
    append(ss, "beautiful!");
    sub = substr(ss, 0, 14);
    printf("%s\n", str(sub));
    free_string(sub);

    char* res = str(ss);
    printf("%s\n", res);

    ss = replace(ss, "at", "@+");
    printf("%s\n", str(ss));


    free_string(ss);

}

void set_test() {

    typedef struct MyStruct {
        int x;
        int y;
    } MyStruct;


    Set* set = new_set();
    
    // s_add objects
    MyStruct object = {1, 3};
    s_any_add(set, &object, sizeof(object));

    // check if has
    int has = s_any_contains(set, &object, sizeof(object));
    assert(has, "has check");

    // iterate over objects
    void** items = set_items(set);
    for (int i = 0; i < set->len; ++i) {
        MyStruct* ptr = (MyStruct*) items[i];

        printf("{%d,%d}\n", ptr->x, ptr->y);
    }

    // s_add more
    int my_int = 10;
    s_int_add(set, my_int);


    // remove
    s_any_erase(set, &object, sizeof(object));

    // get length
    int len = set->len;
    printf("%d\n", len);
    assert(len == 1, "check len");


    // clean up
    free_set(set);
}

int main() {
    // list_test_push_push_front_pop_pop_front_resize_get_set();
    // map_resizing_all_methods();
    // stringstream_test();
    // set_test();
    // return 0;



    CsvDb* db = new_database();
    load_csv(db, "test.csv");

    List* row1 = new_list();
    l_push(row1, "user_123");
    l_push(row1, "bob@gmail.com");
    l_push(row1, "-1");

    List* row2 = new_list();
    l_push(row2, "user_423");
    l_push(row2, "bebo@gmail.com");
    l_push(row2, "5723409234");

    List* rows = new_list();
    l_push(rows, row1);
    l_push(rows, row2);

    Map* table_name_to_rows = new_map();
    char* table_name = "test";
    m_put(table_name_to_rows, table_name, rows, sizeof(rows));

    Map* table_name_to_keys_to_delete = new_map();
    m_put(table_name_to_keys_to_delete, table_name, "user_423", sizeof(char*));
    
    transaction(db, table_name_to_rows, table_name_to_keys_to_delete);

    free_map(table_name_to_rows, 0);
    free_map(table_name_to_keys_to_delete, 0);
    free_list(rows, 0);
    free_list(row1, 0);
    free_list(row2, 0);
    
    free_database(db);



    // String* ss = new_string();
    // append(ss, "hi");
    // char* str1 = free_string_str(ss);
    // ss = new_string();
    // append(ss, "hi2");
    // char* str2 = free_string_str(ss);

    // void** array = malloc(4 * sizeof(void*));
    // array[0] = str1;
    // array[1] = str2;


    // qsort(array, 2, sizeof(void*), compare_file_names);


    return 0;
}
