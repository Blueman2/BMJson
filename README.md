# BMJson
BMJson is a user-friendly, modern C++20 single header JSON library for serialization and deserialization. 

# Requirements
- C++20 or later

# Features
- **Single Header**: Easy to integrate into your project without third party dependencies.
- **Type safe**
- **Comprehensive error handling**: Provides detailed error messages for debugging.
- **User-friendly API**: Intuitive and easy to use.

# Error Reporting
The library provides detailed error messages providing the location and reason for the error.
Example error message:
```
Error at position 192[Anytown]:       "street": "Whatever St",
            "city"  *ERROR*--> "Anytown"
        },
        "courses": [
            {
                "course_name": "Mathematics" 
Error Reason: Expected ':'
```

# Examples
## Deserialization
```cpp
    BMJson::Json Parser{};
    Parser.Parse(FormatedJson);
    
    if(Parser.HasError())
    {
        std::cerr << Parser.GetError() << std::endl << std::endl;
        return;
    }
    
    int Age = Parser["age"];
    bool IsStudent = Parser["is_student"];
    
    BMJson::JsonArray& Grades = Parser["grades"];
    BMJson::JsonObject& Address = Parser["address"];
```
Checking fields
```cpp
    if(!BMJson::HasField<std::string>(Parser, "test"))
    {
        std::cerr << "No test field" << std::endl;
    }
```
It is possible to explicitly cast the field into a specific type using `BMJson::GetField<Type>(JsonObject/Array/Parser, "field")`
```cpp
auto Test = Parser["test"].GetAs<std::string>();
```

## Serialization
Operator [] returns `JsonValueWrapper`
```cpp
    //Set properties on the root object
    BMJson::Json Parser{};
    Parser["Blah"] = "Blah";
    Parser["Blah2"] = 123;
    Parser["Blah3"] = true;

    //json array
    {
        BMJson::JsonArray Array;
        Array.AddValue() = "Blah";
        Array.AddValue() = 123;
        Array.AddValue() = true;

        Parser["Array"] = std::move(Array);
    }
    
    //Nesting
    {
        auto& NewArray = Parser["Array2"].CreateArray();
        NewArray.AddValue() = "Blah";
        NewArray.AddValue() = 123;
        NewArray.AddValue() = true;
        
        auto& NestedObject = NewArray.AddValue().CreateObject();
        NestedObject["Blah"] = "Blah";
        NestedObject["Blah2"] = 123;
        NestedObject["Blah3"] = true;
    }

    //Object
    {
        BMJson::JsonObject Object;
        Object["Blah"] = "Blah";
        Object["Blah2"] = 123;
        Object["Blah3"] = true;

        Parser["Object"] = std::move(Object);
    }

    {
        auto& NewObject = Parser["Object2"].CreateObject();
        NewObject["Blah"] = "Blah";
        NewObject["Blah2"] = 123;
        NewObject["Blah3"] = true;

        auto& NestedArray = NewObject["Array"].CreateArray();
        NestedArray.AddValue() = "Blah";
        NestedArray.AddValue() = 123;

        auto& NestedObject = NestedArray.AddValue().CreateObject();
        NestedObject["Blah"] = "Blah";
        NestedObject["Blah2"] = 123;
    }

    auto Result = Parser.Serialize(true);
    std::cout << Result << std::endl;
```
### Init List
BMJson also supports initializer list syntax for easy creation of JSON objects and arrays.
Init list is supported for `BMJson::JsonObject`, `BMJson::JsonArray`, `BMJson::Json` for both constructors and assignment operator.
```cpp
BMJson::Json Parser{
    {"bool field", true},
    {"float", 4.0f},
    {"Test String", TestString},
    {"test obj",
        {
            {"test", 1.0f},
            {"bool field", true},
            {"big boy array",
                {
                    {"blah"},
                    {1},
                    {2},
                    {
                        {"bool", true}
                    }
                }}
        }},
    {"null field", nullptr}
};
```
### Monadic Operations
Support for `Or, Then, Else` operations
```cpp
        Parser["RandomField"].Else([]()
        {
            std::cout << "No random field" << std::endl;
        });
        
        int64_t Age = Parser["blah"].Or(50).Then<int>([](BMJson::JsonValue& Value)
        {
            std::cout << "Blah field: " << std::get<int64_t>(Value) << std::endl;
        }).Else([]()
        {
            std::cout << "No blah field" << std::endl;
        });
```
