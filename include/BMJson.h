/*
* MIT License

Copyright (c) 2025 BlueMan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <variant>
#include <cctype>
#include <format>
#include <functional>
#include <string>

#define ThrowParserError(...)\
    ThrowError(__VA_ARGS__);\
    return {}
    

namespace BMJson
{
    enum class JsonTokenType
    {
        None,
        ObjectStart,
        ObjectEnd,
        ArrayStart,
        ArrayEnd,
        String,
        Number,
        Boolean,
        Null,
        Comma,
        Colon,
        Invalid
    };

    struct UndefinedValue
    {
    };
    
    struct JsonObject;
    struct JsonArray;
    class Json;
    using JsonValue = std::variant<UndefinedValue, std::nullptr_t, bool, int64_t, double, std::string, std::shared_ptr<JsonArray>, std::shared_ptr<JsonObject>>;
    using TJsonInitList = std::initializer_list<struct JsonInitValue>;

    template<typename T>
    concept CIsValidJsonSetValue = std::is_same_v<T, bool> ||
        std::is_convertible_v<T, int64_t> ||
        std::is_convertible_v<T, double> ||
        std::is_convertible_v<T, std::string> ||
        std::is_same_v<T, std::shared_ptr<JsonArray>> ||
        std::is_same_v<T, std::shared_ptr<JsonObject>>;

    template<typename T>
    concept CIsValidJsonGetValue = std::is_same_v<T, bool> ||
        std::is_same_v<T, int64_t> ||
        std::is_same_v<T, double> ||
        std::is_same_v<T, std::string> ||
        std::is_same_v<T, std::shared_ptr<JsonArray>> ||
        std::is_same_v<T, std::shared_ptr<JsonObject>>;
    
    template<typename T>
    concept CDirectValueInit = !(std::is_integral_v<T> && !std::is_same_v<T, bool>) &&
        !std::is_floating_point_v<T>;

    
    template<typename T>
    struct TJsonValueTypeConverter
    {
        using Type = T;
    };

    template<>
    struct TJsonValueTypeConverter<JsonObject>
    {
        using Type = std::shared_ptr<JsonObject>;
    };

    template<>
    struct TJsonValueTypeConverter<JsonArray>
    {
        using Type = std::shared_ptr<JsonArray>;
    };

    template<typename T>
    requires(!CIsValidJsonGetValue<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>)
    struct TJsonValueTypeConverter<T>
    {
        using Type = int64_t;
    };

    template<typename T>
    requires(!CIsValidJsonGetValue<T> && std::is_floating_point_v<T>)
    struct TJsonValueTypeConverter<T>
    {
        using Type = double;
    };

    
    template<typename T>
    bool HasType(const JsonValue& Value);
    
    template<typename T = void>
    bool HasField(JsonObject& JsonObject, const std::string& Key);

    template<typename T = void>
    bool HasField(const JsonArray& JsonArray, size_t Index);

    template<typename T = void>
    bool HasField(const Json& JsonParser, const std::string& Key);
    
    
    struct JsonInitValue
    {
        struct InitValue
        {
            template<typename T>
            requires(CDirectValueInit<T>)
            InitValue(T&& ValueIn) :
            Value(std::forward<T>(ValueIn))
            {
                
            }

            template<typename T>
            requires(std::is_integral_v<T> && !std::is_same_v<T, bool>)
            InitValue(T ValueIn) :
            Value(static_cast<int64_t>(ValueIn))
            {
                
            }

            template<typename T>
            requires(std::is_floating_point_v<T>)
            InitValue(T ValueIn) :
            Value(static_cast<double>(ValueIn))
            {
                
            }
            
            
            JsonValue Value;
        };
        
        JsonInitValue() = default;
        
        JsonInitValue(std::string KeyIn, InitValue ValueIn);
        JsonInitValue(InitValue ValueIn);

        JsonInitValue(std::string KeyIn, const TJsonInitList& List);
        JsonInitValue(TJsonInitList&& List);

        static JsonValue InitFromList(const TJsonInitList& List, bool bObjectOnly);
        
        
        std::optional<std::string> Key;
        JsonValue Value;
    };

    template<typename T, bool bHasOr = false>
    struct JsonValueWrapper;
    
    struct JsonObject
    {
        JsonObject() = default;

        JsonObject(const TJsonInitList& List)
        {
            InitFromList(List);
        }

        JsonObject& operator=(const TJsonInitList& List)
        {
            InitFromList(List);
            return *this;
        }
        
        JsonValueWrapper<JsonValue> operator[](const std::string& Key);
        JsonValueWrapper<const JsonValue> operator[](const std::string& Key) const;
        
        std::unordered_map<std::string, JsonValue> Properties{};
        
    private:
        void InitFromList(const TJsonInitList& List)
        {
            auto Value = JsonInitValue::InitFromList(List, true);
            if(auto* ObjPtr = std::get_if<std::shared_ptr<JsonObject>>(&Value); ObjPtr && *ObjPtr)
            {
                Properties = std::move((*ObjPtr)->Properties);
            }
        }
    };

    struct JsonArray
    {
        JsonArray() = default;

        JsonArray(const TJsonInitList& List)
        {
            InitFromList(List);
        }
        
        JsonArray& operator=(const TJsonInitList& List)
        {
            InitFromList(List);
            return *this;
        }
        
        JsonValueWrapper<JsonValue> operator[](size_t Index);
        JsonValueWrapper<const JsonValue> operator[](size_t Index) const;
        JsonValueWrapper<JsonValue> AddValue();
        
        std::vector<JsonValue> Values{};
        
    private:
        void InitFromList(const TJsonInitList& List)
        {
            auto Value = JsonInitValue::InitFromList(List, false);
            if(auto* ObjPtr = std::get_if<std::shared_ptr<JsonArray>>(&Value); ObjPtr && *ObjPtr)
            {
                Values = std::move((*ObjPtr)->Values);
            }
        }
    };

    template<typename TJsonValue, bool bHasOr>
    struct JsonValueWrapper
    {
        static constexpr bool bIsConst = std::is_const_v<TJsonValue>;

        template<typename T>
        using TType = std::conditional_t<bIsConst, const T, T>;
        
        template<typename T>
        using TReturnType = std::conditional_t<bHasOr, T, TType<T&>>;

        struct EmptyDefault {};
        struct Default
        {
            JsonValue Value;
        };

        using DefaultType = std::conditional_t<bHasOr, Default, EmptyDefault>;
        
        JsonValueWrapper(TJsonValue& Value) :
        Value(Value)
        {
            
        }

        JsonValueWrapper<TJsonValue, true> Or(JsonInitValue::InitValue OrInit) requires(!bHasOr)
        {
            JsonValueWrapper<TJsonValue, true> OrWrapper{Value};
            OrWrapper.DefaultValue.Value = std::move(OrInit.Value);

            return OrWrapper;
        }

        template<typename T = void>
        JsonValueWrapper& Then(const std::function<void(TType<JsonValue&>)>& Func)
        {
            if constexpr(std::is_same_v<T, void>)
            {
                if(!HasField<UndefinedValue>(Value))
                {
                    Func(Value);
                }
                else
                {
                    if constexpr(bHasOr)
                    {
                        if(!HasType<UndefinedValue>(DefaultValue.Value))
                        {
                            Func(DefaultValue.Value);
                        }
                    }
                }
            }
            else
            {
                if(HasType<T>(Value))
                {
                    Func(Value);
                }
                else
                {
                    if constexpr(bHasOr)
                    {
                        if(HasType<T>(DefaultValue.Value))
                        {
                            Func(DefaultValue.Value);
                        }
                    }
                }
            }

            return *this;
        }

        JsonValueWrapper& Else(const std::function<void()>& Func)
        {
            bool bUndefined = HasType<UndefinedValue>(Value);
            if constexpr(bHasOr)
            {
                bUndefined &= HasType<UndefinedValue>(DefaultValue.Value);
            }

            if(bUndefined)
            {
                Func();
            }

            return *this;
        }

        template<typename T>
        auto GetAs() -> TReturnType<typename TJsonValueTypeConverter<T>::Type>
        {
            using TValue = typename TJsonValueTypeConverter<T>::Type;
            return Get_Internal<TValue>();
        }
        
        JsonObject& CreateObject() requires(!bIsConst && !bHasOr)
        {
            if(!HasType<JsonObject>(Value))
            {
                Value = std::make_shared<JsonObject>();
            }

            return *std::get<std::shared_ptr<JsonObject>>(Value);
        }

        JsonArray& CreateArray() requires(!bIsConst && !bHasOr)
        {
            if(!HasType<JsonArray>(Value))
            {
                Value = std::make_shared<JsonArray>();
            }

            return *std::get<std::shared_ptr<JsonArray>>(Value);
        }

        JsonValueWrapper& operator=(JsonObject&& ValueIn) requires(!bHasOr)
        {
            Value = std::make_shared<JsonObject>(std::move(ValueIn));
            return *this;
        }

        JsonValueWrapper& operator=(TJsonInitList List) requires(!bHasOr)
        {
            Value = JsonInitValue::InitFromList(List, false);
            return *this;
        }

        JsonValueWrapper& operator=(JsonArray&& ValueIn) requires(!bHasOr)
        {
            Value = std::make_shared<JsonArray>(std::move(ValueIn));
            return *this;
        }
        
        template<typename T>
        requires(CIsValidJsonSetValue<T>)
        JsonValueWrapper& operator=(T&& ValueIn) requires(!bHasOr)
        {
            Value = std::forward<T>(ValueIn);
            return *this;
        }

        operator TReturnType<JsonArray>()
        {
            auto ObjPtr = Get_Internal<std::shared_ptr<JsonArray>>();
            if(!ObjPtr)
            {
                throw std::runtime_error("Field is not a JsonArray");
            }

            return *ObjPtr;
        }

        operator TReturnType<JsonObject>()
        {
            auto ObjPtr = Get_Internal<std::shared_ptr<JsonObject>>();
            if(!ObjPtr)
            {
                throw std::runtime_error("Field is not a JsonObject");
            }

            return *ObjPtr;
        }
        
        template<typename T>
        requires(CIsValidJsonGetValue<T>)
        operator T&() requires(!bIsConst && !bHasOr)
        {
            return Get_Internal<T>();
        }

        template<typename T>
        requires(CIsValidJsonGetValue<T>)
        operator const T&() requires(bIsConst && !bHasOr)
        {
            return Get_Internal<T>();
        }

        template<typename T>
        requires(CIsValidJsonGetValue<T>)
        operator T() requires(bHasOr)
        {
            return Get_Internal<T>();
        }

        template<typename T>
        requires(!CIsValidJsonGetValue<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>)
        operator T()
        {
            return Get_Internal<int64_t>();
        }

        template<typename T>
        requires(!CIsValidJsonGetValue<T> && std::is_floating_point_v<T>)
        operator T()
        {
            return Get_Internal<double>();
        }

        
        DefaultType DefaultValue{};
        
    private:
        template<typename T>
        auto Get_Internal() -> std::conditional_t<bIsConst, const T&, T&> requires(!bHasOr)
        {
            if constexpr(bIsConst)
            {
                if(!HasType<T>(Value))
                {
                    throw std::runtime_error("Field is not of the requested type");
                }
            }
            else
            {
                if(!HasType<T>(Value))
                {
                    Value = T{};
                }
            }

            return std::get<T>(Value);
        }

        template<typename T>
        auto Get_Internal() -> std::conditional_t<bIsConst, const T&, T&> requires(bHasOr)
        {
            if(!HasType<T>(Value))
            {
                if(!HasType<T>(DefaultValue.Value)) 
                {
                    throw std::runtime_error("Or value is not of the requested type");
                }

                return std::get<T>(DefaultValue.Value);
            }

            return std::get<T>(Value);
        }
        
        TJsonValue& Value;
    };


    struct JsonToken
    {
        JsonToken() = default;
        JsonToken(const JsonToken& Other) = default;
        JsonToken& operator=(const JsonToken& Other) = default;

        JsonToken(JsonTokenType Number, size_t Size, std::string Str) :
        Type(Number),
        Position(Size),
        Value(std::move(Str))
        {
        
        }

        JsonToken(JsonToken&& Other) :
        Type(Other.Type),
        Position(Other.Position),
        Value(std::move(Other.Value))
        {
            Other.Type = JsonTokenType::Invalid;
            Other.Position = 0;
        }

        JsonToken& operator=(JsonToken&& Other)
        {
            if(this != &Other)
            {
                Type = Other.Type;
                Position = Other.Position;
                Value = std::move(Other.Value);
                
                Other.Type = JsonTokenType::Invalid;
                Other.Position = 0;
            }
            return *this;
        }
    

        JsonTokenType Type{JsonTokenType::Invalid};
        size_t Position{};
        std::string Value{};
    };
    
    
    class JsonTokenizer
    {
    public:
        JsonTokenizer() = default;
        JsonTokenizer(const JsonTokenizer& Other) = default;
        JsonTokenizer& operator=(const JsonTokenizer& Other) = default;
        JsonTokenizer(JsonTokenizer&& Other) = default;
        JsonTokenizer& operator=(JsonTokenizer&& Other) = default;

        void Init(std::string_view InputIn)
        {
            Input = InputIn;
            Position = 0;
            CurrentToken = {JsonTokenType::Invalid, 0, ""};
        }

        JsonToken PeekToken()
        {
            if(CurrentToken.Type == JsonTokenType::Invalid)
            {
                CurrentToken = NextToken();
            }
        
            return CurrentToken;
        }

        JsonToken GetToken()
        {
            if(CurrentToken.Type == JsonTokenType::Invalid)
            {
                CurrentToken = NextToken();
            }
        
            auto Token = CurrentToken;
            CurrentToken = NextToken();

            return Token;
        }

        [[nodiscard]] std::string_view GetInput() const
        {
            return Input;
        }


    private:
        static bool IsValidNumberChar(char c)
        {
            return std::isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E';
        }
    
        JsonToken NextToken()
        {
            SkipWhitespace();
            if(Position >= Input.size())
            {
                return {JsonTokenType::None, Position, ""};
            }

            const size_t TokenPosition = Position;
            switch(const char Current = Peek())
            {
                case '{': return {JsonTokenType::ObjectStart, TokenPosition, {Get()}};
                case '}': return {JsonTokenType::ObjectEnd, TokenPosition, {Get()}};
                case '[': return {JsonTokenType::ArrayStart, TokenPosition, {Get()}};
                case ']': return {JsonTokenType::ArrayEnd, TokenPosition, {Get()}};
                case ',': return {JsonTokenType::Comma, TokenPosition, {Get()}};
                case ':': return {JsonTokenType::Colon, TokenPosition, {Get()}};
                case 'n': return ParseNull();
                case '"': return ParseString();
                case 't' : case 'f': return ParseBoolean();
                
                default:
                {
                    if(IsValidNumberChar(Current))
                    {
                        return ParseNumer();
                    }
                }
            }

            return {JsonTokenType::Invalid, Position, ""};
        }

        JsonToken ParseNull()
        {
            JsonToken token{JsonTokenType::Invalid, Position, ""};
            if(Input.substr(Position, 4) == "null")
            {
                token.Value = "null";
                token.Type = JsonTokenType::Null;
                Position += 4;
            }
            return token;
        }
    
        JsonToken ParseString()
        {
            JsonToken Token{JsonTokenType::String, Position, ""};
            Get();

            for(char Current = Get(); Current != '\0'; Current = Get())
            {
                if(Current == '\\')
                {
                    // Handle escape sequences
                    Token.Value += Get();
                    continue;
                }

                if(Current == '"') break;
                Token.Value += Current;
            }

            return Token;
        }

        JsonToken ParseNumer()
        {
            JsonToken token{JsonTokenType::Number, Position, ""};
            for(char Current = Peek(); IsValidNumberChar(Current); Current = Peek())
            {
                token.Value += Current;
                Get();
            }

            return token;
        }

        JsonToken ParseBoolean()
        {
            JsonToken token{JsonTokenType::Invalid, Position, ""};
            if(Input.substr(Position, 4) == "true")
            {
                token.Value = "true";
                token.Type = JsonTokenType::Boolean;
                Position += 4;
            }
            else if(Input.substr(Position, 5) == "false")
            {
                token.Value = "false";
                token.Type = JsonTokenType::Boolean;
                Position += 5;
            }
            return token;
        }
    
        void SkipWhitespace()
        {
            while (Position < Input.size() && std::isspace(Input[Position]))
            {
                ++Position;
            }
        }
    
        [[nodiscard]] char Peek() const
        {
            return Position < Input.size() ? Input[Position] : '\0';
        }

        [[nodiscard]] char PeekAhead(size_t Offset) const
        {
            const size_t NewPosition = Position + Offset;
            return NewPosition < Input.size() ? Input[NewPosition] : '\0';
        }

        char Get()
        {
            return Position < Input.size() ? Input[Position++] : '\0';
        }
    
        size_t Position{};
        std::string_view Input{};
        JsonToken CurrentToken{};
    };
    
    class Json
    {
    public:
        Json()
        {
            RootObject = std::make_shared<JsonObject>();
        }

        Json(const Json& Other) :
        Tokenizer{Other.Tokenizer},
        CurrentToken{Other.CurrentToken},
        ErrorMessage{Other.ErrorMessage}
        {
            if(Other.RootObject)
            {
                RootObject = std::make_shared<JsonObject>();
                *RootObject = *Other.RootObject;
            }
        }

        Json(Json&& Other) :
        Tokenizer{std::move(Other.Tokenizer)},
        CurrentToken{std::move(Other.CurrentToken)},
        ErrorMessage{std::move(Other.ErrorMessage)},
        RootObject{std::move(Other.RootObject)}
        {
            Other.Tokenizer.Init("");
            Other.CurrentToken = {JsonTokenType::Invalid, 0, ""};
            Other.ErrorMessage.reset();
        }

        Json& operator=(const Json& Other)
        {
            if(this != &Other)
            {
                Tokenizer = Other.Tokenizer;
                CurrentToken = Other.CurrentToken;
                ErrorMessage = Other.ErrorMessage;

                if(Other.RootObject)
                {
                    RootObject = std::make_shared<JsonObject>();
                    *RootObject = *Other.RootObject;
                }
            }
            return *this;
        }

        Json& operator=(Json&& Other)
        {
            if(this != &Other)
            {
                Tokenizer = std::move(Other.Tokenizer);
                CurrentToken = std::move(Other.CurrentToken);
                ErrorMessage = std::move(Other.ErrorMessage);
                RootObject = std::move(Other.RootObject);

                Other.Tokenizer.Init("");
                Other.CurrentToken = {JsonTokenType::Invalid, 0, ""};
                Other.ErrorMessage.reset();
            }
            return *this;
        }

        Json(const TJsonInitList& List)
        {
            InitFromList(List);
        }

        Json& operator=(const TJsonInitList& List)
        {
            InitFromList(List);
            return *this;
        }

        JsonValueWrapper<JsonValue> operator[](const std::string& Key)
        {
            if(!RootObject)
            {
                RootObject = std::make_shared<JsonObject>();
            }

            auto& Value = RootObject->Properties[Key];
            return {Value};
        }

        JsonValueWrapper<const JsonValue> operator[](const std::string& Key) const
        {
            if(!RootObject) throw std::runtime_error("Root object is null, const access not possible");
            
            auto& Value = RootObject->Properties[Key];
            return {Value};
        }

        void Reset(bool bCreateRoot = true)
        {
            if(bCreateRoot)
            {
                if(RootObject)
                {
                    RootObject->Properties.clear();
                }
                else
                {
                    RootObject = std::make_shared<JsonObject>();
                }
            }
            
            Tokenizer.Init("");
            ErrorMessage.reset();
            CurrentToken = {JsonTokenType::Invalid, 0, ""};
        }

        void Parse(std::string_view Input)
        {
            Tokenizer.Init(Input);
            ErrorMessage.reset();
            
            RootObject = ParseObject();
        }

        [[nodiscard]] std::string Serialize(bool bPretty) const
        {
            std::string Result;
            if(!RootObject) return Result;

            SerializeObject(*RootObject, Result, bPretty, 0);
            return Result;
        }
        
        [[nodiscard]] bool HasError() const
        {
            return ErrorMessage.has_value();
        }

        [[nodiscard]] std::string_view GetError() const
        {
            if(ErrorMessage.has_value())
            {
                return *ErrorMessage;
            }
            return "";
        }

        [[nodiscard]] const std::shared_ptr<JsonObject>& GetRootObject() const
        {
            return RootObject;
        }

    private:
        void InitFromList(const TJsonInitList& List)
        {
            if(!RootObject)
            {
                RootObject = std::make_shared<JsonObject>();
            }

            *RootObject = List;
        }
        
        void Peek()
        {
            UpdateToken(true);
        }

        void Consume()
        {
            UpdateToken(false);
        }

        void UpdateToken(bool bPeak)
        {
            CurrentToken = bPeak ? Tokenizer.PeekToken() : Tokenizer.GetToken();
            if(CurrentToken.Type == JsonTokenType::Invalid)
            {
                ThrowError(CurrentToken, "Invalid token");
            }
        }

        //Serialization
        void SerializeValue(const JsonValue& Value, std::string& Result, bool bPretty, size_t Depth) const;
        void SerializeArray(const JsonArray& Array, std::string& Result, bool bPretty, size_t Depth) const;
        void SerializeObject(const JsonObject& Object, std::string& Result, bool bPretty, size_t Depth) const;

        //Deserialization
        JsonValue ParseValue();
        std::shared_ptr<JsonArray> ParseArray();
        std::shared_ptr<JsonObject> ParseObject();
        
        void ThrowError(const JsonToken& Token, const std::string& message);
    
    
        JsonTokenizer Tokenizer;
        JsonToken CurrentToken{};
        std::optional<std::string> ErrorMessage{}; 
        std::shared_ptr<JsonObject> RootObject;
    };

    inline void Json::SerializeValue(const JsonValue& Value, std::string& Result, bool bPretty, size_t Depth) const
    {
        if(HasType<int64_t>(Value))
        {
            auto& Number = std::get<int64_t>(Value);
            Result = std::to_string(Number);
        }
        else if(HasType<double>(Value))
        {
            auto& Number = std::get<double>(Value);
            Result = std::to_string(Number);
        }
        else if(HasType<nullptr_t>(Value))
        {
           Result = "null";
        }
        else if(HasType<bool>(Value))
        {
            auto& Bool = std::get<bool>(Value);
            Result = Bool ? "true" : "false";
        }
        else if(HasType<std::string>(Value))
        {
            auto& String = std::get<std::string>(Value);
            Result = "\"" + String + "\"";
        }
        else if(HasType<JsonArray>(Value))
        {
            SerializeArray(*std::get<std::shared_ptr<JsonArray>>(Value), Result, bPretty, Depth + 1);
        }
        else if(HasType<JsonObject>(Value))
        {
            SerializeObject(*std::get<std::shared_ptr<JsonObject>>(Value), Result, bPretty, Depth + 1);
        }
    }

    inline void Json::SerializeArray(const JsonArray& Array, std::string& Result, bool bPretty, size_t Depth) const
    {
        Result += '[';

        size_t Written{};
        for(const auto& Value : Array.Values)
        {
            if(Written > 0)
            {
                Result += ",";
            }

            if(bPretty)
            {
                Result += "\n";
                Result += std::string(Depth + 1, '\t');
            }

            std::string ValueString;
            SerializeValue(Value, ValueString, bPretty, Depth);
            Result += ValueString;

            ++Written;
        }

        if(bPretty && Result.size() > 1)
        {
            Result += "\n";
            Result += std::string(Depth, '\t');
        }
        
        Result += "]";
    }

    inline void Json::SerializeObject(const JsonObject& Object, std::string& Result, bool bPretty, size_t Depth) const
    {
        Result += '{';

        size_t Written{};
        for(const auto& [Key, Value] : Object.Properties)
        {
            if(Written > 0)
            {
                Result += ",";
            }

            std::string Separator = " ";
            if(bPretty)
            {
                Result += "\n";
                Result += std::string(Depth + 1, '\t');

                if(HasType<JsonObject>(Value) || HasType<JsonArray>(Value))
                {
                    Separator = "\n" + std::string(Depth + 1, '\t');
                }
            }

            std::string ValueString;
            SerializeValue(Value, ValueString, bPretty, Depth);

            Result += std::format("\"{}\":{}{}", Key, Separator, ValueString);
            ++Written;
        }

        if(bPretty && Result.size() > 1)
        {
            Result += "\n";
            Result += std::string(Depth, '\t');
        }

        Result += "}";
    }

    inline JsonValue Json::ParseValue()
    {
        Peek();
        switch(CurrentToken.Type)
        {
            case JsonTokenType::ObjectStart:
            {
                return ParseObject();
            }
            case JsonTokenType::ArrayStart:
            {
                return ParseArray();
            }
            case JsonTokenType::String:
            {
                Consume();
                return CurrentToken.Value;
            }
            case JsonTokenType::Number:
            {
                auto IsFloat = [&]()
                {
                    return CurrentToken.Value.find('.') != std::string::npos || CurrentToken.Value.find('e') != std::string::npos;
                };
                
                Consume();
                if(IsFloat())
                {
                    return std::stod(CurrentToken.Value);
                }
                else
                {
                    return std::stoll(CurrentToken.Value);
                }
            }
            case JsonTokenType::Null:
            {
                Consume();
                return nullptr;
            }
            case JsonTokenType::Boolean:
            {
                Consume();
                return CurrentToken.Value == "true";
            }
            default:;
        }

        ThrowParserError(CurrentToken, "Unexpected token while parsing value");
    }

    inline std::shared_ptr<JsonArray> Json::ParseArray()
    {
        {
            Consume();
            if(CurrentToken.Type != JsonTokenType::ArrayStart)
            {
                ThrowParserError(CurrentToken, "Expected '['");
            }

            Peek();
            std::shared_ptr<JsonArray> Result = std::make_shared<JsonArray>();
            auto& Values = Result->Values;

            if(CurrentToken.Type == JsonTokenType::ArrayEnd)
            {
                Consume();
                return Result;
            }
            
            for(;; Peek())
            {
                auto Value = ParseValue();
                if(HasError()) return {};
                
                Values.push_back(std::move(Value));

                Consume();
                if(CurrentToken.Type != JsonTokenType::Comma && CurrentToken.Type != JsonTokenType::ArrayEnd)
                {
                    ThrowParserError(CurrentToken, "Expected ',' or ']'");
                }

                if(CurrentToken.Type == JsonTokenType::ArrayEnd) break;
            }

            return Result;
        }
    }

    inline std::shared_ptr<JsonObject> Json::ParseObject()
    {
        Consume();
        if(CurrentToken.Type != JsonTokenType::ObjectStart)
        {
            ThrowParserError(CurrentToken, "Expected '{'");
        }

        Peek();
        std::shared_ptr<JsonObject> Result = std::make_shared<JsonObject>();

        if(CurrentToken.Type == JsonTokenType::ObjectEnd)
        {
            Consume();
            return Result;
        }
        
        for(;; Peek())
        {
            if(CurrentToken.Type != JsonTokenType::String)
            {
                ThrowParserError(CurrentToken, "Expected string key");
            }

            // Get the key
            Consume();
            auto Key = CurrentToken.Value;

            // Get the colon
            Consume();
            if(CurrentToken.Type != JsonTokenType::Colon)
            {
                ThrowParserError(CurrentToken, "Expected ':'");
            }

            // Parse the value
            auto Value = ParseValue();
            if(HasError()) return {};
                
            Result->Properties.emplace(std::move(Key), std::move(Value));

            Consume();
            if(CurrentToken.Type != JsonTokenType::Comma && CurrentToken.Type != JsonTokenType::ObjectEnd)
            {
                ThrowParserError(CurrentToken, "Expected ',' or '}'");
            }

            if(CurrentToken.Type == JsonTokenType::ObjectEnd) break;
        }

        return Result;
    }

    inline void Json::ThrowError(const JsonToken& Token, const std::string& message)
    {
        if(HasError()) return;
            
        std::string_view Input = Tokenizer.GetInput();
        std::string ErrorLocation;
        if(Token.Position >= Input.size())
        {
            ErrorLocation = "Error position out of bounds";
        }
        else
        {
            static constexpr size_t MaxErrorLocation = 50;
                
            const size_t Start = Token.Position >= MaxErrorLocation ? Token.Position - MaxErrorLocation : 0;
            const size_t Diff = Token.Position - Start;
            const size_t End = std::min<size_t>(Token.Position + MaxErrorLocation + Diff, Input.size());
            
            ErrorLocation = std::string(Input.substr(Start, End - Start));
            if(Diff > 0)
            {
                ErrorLocation = std::string(ErrorLocation.substr(0, Diff)) + " *ERROR*--> " + std::string(ErrorLocation.substr(Diff));
            }
        }
        
        ErrorMessage = std::format("Error at position {}[{}]: {} \nError Reason: {}", Token.Position, Token.Value, ErrorLocation, message);
    }
    

    template<typename T>
    bool HasType(const JsonValue& Value)
    {
        using TType = typename TJsonValueTypeConverter<T>::Type;
        return std::holds_alternative<TType>(Value);
    }
    
    template<typename T>
    bool HasField(JsonObject& JsonObject, const std::string& Key)
    {
        const auto It = JsonObject.Properties.find(Key);
        if(It != JsonObject.Properties.end())
        {
            if constexpr(std::is_same_v<T, void>)
            {
                return true;
            }
            else
            {
                return HasType<T>(It->second);
            }
        }

        return false;
    }

    template<typename T>
    bool HasField(const JsonArray& JsonArray, size_t Index)
    {
        if(Index >= JsonArray.Values.size()) return false;
        if constexpr(std::is_same_v<T, void>)
        {
            return true;
        }
        else
        {
            return HasType<T>(JsonArray.Values[Index]);
        }
    }

    template<typename T>
    bool HasField(const Json& JsonParser, const std::string& Key)
    {
        if(!JsonParser.GetRootObject()) return false;
        return HasField<T>(*JsonParser.GetRootObject(), Key);
    }
    
    
    inline JsonInitValue::JsonInitValue(std::string KeyIn, const TJsonInitList& List) :
    Key(std::move(KeyIn))
    {
        Value = InitFromList(List, false);
    }

    inline JsonInitValue::JsonInitValue(TJsonInitList&& List)
    {
        Value = InitFromList(List, false);
    }
    
    inline JsonInitValue::JsonInitValue(std::string KeyIn, InitValue ValueIn) :
    Key(std::move(KeyIn)),
    Value(std::move(ValueIn.Value))
    {
    }
    
    inline JsonInitValue::JsonInitValue(InitValue ValueIn) :
    Value(std::move(ValueIn.Value))
    {
    }

    inline JsonValue JsonInitValue::InitFromList(const TJsonInitList& List, bool bObjectOnly)
    {
        if(List.size() < 1) return {};

        const bool bIsObject = List.begin()->Key.has_value();
        JsonValue Value;
        if(bIsObject)
        {
            auto Obj = std::make_shared<JsonObject>();
            for(const auto& InitValue : List)
            {
                if(!InitValue.Key.has_value()) continue;
                Obj->Properties.emplace(*InitValue.Key, InitValue.Value);
            }

            Value = std::move(Obj);
        }
        else if(!bObjectOnly)
        {
            auto Array = std::make_shared<JsonArray>(); 
            for(const auto& InitValue : List)
            {
                if(InitValue.Key.has_value()) continue;
                Array->Values.push_back(InitValue.Value);
            }

            Value = std::move(Array);
        }

        return std::move(Value);
    }

    
    inline JsonValueWrapper<JsonValue> JsonObject::operator[](const std::string& Key)
    {
        auto& Value = Properties[Key];
        return {Value};
    }

    inline JsonValueWrapper<const JsonValue> JsonObject::operator[](const std::string& Key) const
    {
        if(auto It = Properties.find(Key); It != Properties.end())
        {
            return {It->second};
        }
        
        static JsonValue EmptyValue = UndefinedValue{};
        return {EmptyValue};
    }

    inline JsonValueWrapper<JsonValue> JsonArray::operator[](size_t Index)
    {
        auto& Value = Values.at(Index);
        return {Value};
    }

    inline JsonValueWrapper<const JsonValue> JsonArray::operator[](size_t Index) const
    {
        const auto& Value = Values.at(Index);
        return {Value};
    }

    inline JsonValueWrapper<JsonValue> JsonArray::AddValue()
    {
        Values.emplace_back();
        auto& Value = Values.back();
        
        return {Value};
    }
}

#undef ThrowParserError