#include "var.h"
#include <fstream>
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <dirent.h>
#endif

#include "parse.h"

using namespace base;

void showIter(Variant& v);

void showBasics(const char* str)
{
    Variant v0;
    Variant v1 = 23;
    Variant v2;
    Variant v3;

    // Assign/modifie values
    v0.format("-printf-style %s", str);

    v2 = 553.2f;

    v2 = (double)v2 * 2.0f;

    // Concat
    v3 = v1.toString() + " Hello world ";

    printf("%s\n", v3.toString().c_str());

}

void showSimple()
{
    Variant arr;

    // Create an array in this variant

    arr.createArray();

    // Push some items.  You can also add using append() or insert()

    arr.push(10);
    arr.push(21);
    arr.push(50);
    arr.push(3022);
    arr.push(44);

    arr[1] = 234.00f;
    arr[2] = "Hello world";

    // Iterate over all of the array items (note: unusual syntax).  One can also
    // use length() and [] to iterate over the array.

    for (Iter<Variant> i; arr.forEach(i); )
    {
        printf("%d %s\n", i.pos(), i->toString().c_str());
    }

    showIter(arr);

    // pop() all elements from the array

    Variant p;
    while (!(p = arr.pop()).empty())
    {
        printf("Pop %s\n", p.toString().c_str());
    }
}

void showAltInit()
{
    Variant arr;

    // Init an array using js style initializer syntax (string)

    arr.createArray("[123, 23, 'can be string', 233.2, false, -20, -120]");

    for (int i = 0; i < arr.length(); i++)
    {
        printf("%d %s\n", i, arr[i].toString().c_str());
    }

}

void showArrOfArr()
{
    Variant arr;

    // Arrays can contain any number of any types.  Unlike JS, they cannot have holes, though
    // the element can be of empty type.  Arrays can be nested as well.

    arr.createArray("[0, 'one', '2.0', 3, 'four']");

    arr[3].createArray();
    for (int i = 0; i < 4; i++)
    {
        arr[3].push(i);
    }

    arr[3][2].createArray("[999, 9999, 99999]");

    // Print the entire array of array as string

    printf("%s\n", arr.toString().c_str());

    // Printed:
    // [0,"one","2.0",[0,1,[999,9999,99999],3],"four"]

}

void showSimpleJson()
{
    const char* jsontxt =
        "{"
        "    \"name\": \"manuscript found in accra\","
        "    \"id\": 9781460700297,"
        "    \"price\": 12.50"
        "}";

    Variant v;

    // Parse json text into a variant
    if (v.parseJson(jsontxt))
    {
        // Print the variant as string
        printf("Parsed...\ntoString=%s\n\n", v.toString().c_str());

        // Print the variant as jsonstring (produces strict json)
        printf("toJsonString=%s\n", v.toJsonString().c_str());
    }
}

#ifdef _MSC_VER
void showJsonTestSuite()
{
    const char* datadir = "data\\jsontest\\";

    printf("\nRunning test on json files in %s....\n", datadir);

    WIN32_FIND_DATA ffd;
    std::string dir(datadir);
    dir += "*";
    HANDLE hfind = FindFirstFile(dir.c_str(), &ffd);
    if (hfind == INVALID_HANDLE_VALUE)
    {
        printf("Error: failed to open jsontest directory, err=%lu\n", GetLastError());
        return;
    }

    do
    {
        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            std::string fn(datadir);
            fn += ffd.cFileName;

            printf("\nFilename: '%s' should %s\n", fn.c_str(), fn.find("pass") != std::string::npos ? "pass" : "fail");

            // File f;
            // f.open(fn.c_str(), __f_open::read);
            // auto jsontxt = f.readEntire(true);
            // f.close();

            base::Buffer jsontxt;
            jsontxt.readFile(fn, true);

            Variant v;
            if (v.parseJson((const char*)jsontxt.cptr()))
            {
                printf("PASS!!\n");
            }
            else
            {
                printf("FAIL\n");
            }
        }
   }
   while (FindNextFile(hfind, &ffd) != 0);
   FindClose(hfind);
}

#else
void showJsonTestSuite()
{
    DIR* dir;
    struct dirent* fil;

    const char* datadir = "../data/jsontest/";
    printf("\nRunning test on json files in %s directory....\n", datadir);

    dir = opendir(datadir);
    if (dir == NULL)
    {
        printf("Error: failed to open jsontest directory\n");
        closedir(dir);
        return;
    }

    while ((fil = readdir(dir)) != NULL)
    {
        if (!strieql(fil->d_name, ".") && !strieql(fil->d_name, ".."))
        {
            std::string fn(datadir);

            fn += fil->d_name;

            printf("\nFilename: '%s' should %s\n", fn.c_str(), fn.find("pass") != std::string::npos ? "pass" : "fail");

            Buffer jsontxt;
            jsontxt.readFile(fn.c_str(), true);

            Variant v;
            if (v.parseJson((const char*)jsontxt.cptr()))
            {
                printf("PASS!!\n");
            }
            else
            {
                printf("FAIL\n");
            }
        }
    }

    closedir(dir);
}

#endif


void showIter(Variant& v)
{
    for (Iter<Variant> i; v.forEach(i); )
    {
        printf("Unorth iter: %d. Key: %s Value: %s\n", i.pos(), i.key(), i->toString().c_str());
    }
    printf("\n");

    if (v.isObject())
    {
        int pos = 0;
        for (auto& i : v)
        {
            printf("Auto iter [%s] = '%s'\n", v.getKey(pos), i.toString().c_str());
            pos++;
        }
    }
    else
    {
        for (auto& i : v)
        {
            printf("Auto iter '%s'\n", i.toString().c_str());
        }
    }

    printf("\n");
}

void showSimpleObj()
{
    Variant obj;

    // Create an object in the variant

    obj.createObject();

    // Add properties to the object.  Unlike JS, properties must be explicitly added

    obj.setProp("PropA", 65);
    obj.setProp("PropB", 66);
    obj.setProp("PropC", 67);
    obj.setProp("A");

    // Update propertie values

    obj["PropA"] = 6500;
    obj["PropC"] = (double)obj["PropC"] * 2.0f;
    obj["A"] = 100;

    // Print the entire object as string

    printf("%s\n", obj.toString().c_str());

    // Printed:
    // {"PropA":6500,"PropB":67,"PropC":134.0}

    printf("Iterate\n");
    showIter(obj);

    // Remove a property

    obj.removeProp("PropB");

    printf("%s\n", obj.toString().c_str());

    printf("Iterate after remove\n");
    showIter(obj);
}

void showAltInitObj()
{
    Variant obj;

    obj.createObject("{firstname:'Yasser', lastname:'Asmi', email:'yaasmi@microsoft.com', dogname:'Jake'}");

    showIter(obj);
}

void showObjOfArr()
{
    Variant obj;

    obj.createObject();

    obj.setProp("PropA");
    obj.setProp("PropB");
    obj.setProp("PropC");

    obj["PropA"].createArray("[110, 120, 130]");
    obj["PropB"].createArray("[210, 220]");
    obj["PropC"].createArray("[310, 320, 330, 340, 350]");

    showIter(obj);
}

void jsonTests(int, char** argv)
{
    showBasics(argv[0]);
    showSimple();
    showAltInit();
    showArrOfArr();
    showSimpleJson();
    showJsonTestSuite();
    showAltInit();
    showObjOfArr();
    showSimpleObj();
    showAltInitObj();
    showObjOfArr();
}


void yamlTests(int , char** )
{
#ifdef _MSC_VER
    const char* datadir = "data\\";
#else
    const char* datadir = "data/";
#endif

    Variant out;
    std::string fn;
    YamlParser parser;

    fn = std::string(datadir) + "test1.yaml";
    
    parser.readFile(fn.c_str());
    parser.parse(out);

    printf("File:%s\n%s\n", fn.c_str(), out.toJsonString().c_str());

    fn = std::string(datadir) + "test2.yaml";
    parser.readFile(fn.c_str());
    parser.parse(out);
    printf("File:%s\n%s\n", fn.c_str(), out.toJsonString().c_str());
}

int main(int argc, char** argv)
{
    char* p = (char*)malloc(1024);
    jsonTests(argc, argv);
    yamlTests(argc, argv);
}
