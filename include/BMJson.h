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
#include <bit>
#include <charconv>
#include <cmath>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <variant>
#include <format>
#include <functional>
#include <string>

#if defined(_M_X64) || defined(__x86_64__)
    #define SIMD_SUPPORTED 1
#else
    #define SIMD_SUPPORTED 0
#endif

#define ThrowParserError(...)\
    ThrowError(__VA_ARGS__);\
    return {}
    
#if SIMD_SUPPORTED
    #include <emmintrin.h>
#endif

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
        StringEscaped,
        Number,
        Boolean,
        Null,
        Comma,
        Colon,
        NotSet,
        Error
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

    JsonValue DeepCopyValue(const JsonValue& Source);
    
    
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

    JsonValue DeepCopyValue(const JsonValue& Source);

    std::unordered_map<std::string, JsonValue> DeepCopyMap(const std::unordered_map<std::string, JsonValue>& Source);

    std::vector<JsonValue> DeepCopyArray(const std::vector<JsonValue>& Source);

    struct JsonObject
    {
        JsonObject()
        {
            Properties.reserve(3);
        }

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
                Properties = DeepCopyMap((*ObjPtr)->Properties);
            }
        }
    };

    struct JsonArray
    {
        JsonArray()
        {
            Values.reserve(3);
        }

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
                Values = DeepCopyArray((*ObjPtr)->Values);
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
                if(!HasType<UndefinedValue>(Value))
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
            if(!HasType<JsonObject>(Value) || !std::get<std::shared_ptr<JsonObject>>(Value))
            {
                Value = std::make_shared<JsonObject>();
            }

            return *std::get<std::shared_ptr<JsonObject>>(Value);
        }

        JsonArray& CreateArray() requires(!bIsConst && !bHasOr)
        {
            if(!HasType<JsonArray>(Value) || !std::get<std::shared_ptr<JsonArray>>(Value))
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
            if(!HasType<T>(Value))
            {
                throw std::runtime_error("Field is not of the requested type");
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
        
        JsonToken(JsonTokenType Type, size_t Position, std::string_view Str) :
        Type(Type),
        Position(Position),
        Value(Str)
        {
        
        }

        JsonToken(JsonToken&& Other) :
        Type(Other.Type),
        Position(Other.Position),
        Value(Other.Value)
        {
            Other.Type = JsonTokenType::NotSet;
            Other.Position = 0;
        }

        JsonToken& operator=(JsonToken&& Other)
        {
            if(this != &Other)
            {
                Type = Other.Type;
                Position = Other.Position;
                Value = Other.Value;
                
                Other.Type = JsonTokenType::NotSet;
                Other.Position = 0;
            }
            return *this;
        }
    

        JsonTokenType Type{JsonTokenType::NotSet};
        size_t Position{};
        std::string_view Value{};
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
            CurrentToken = {JsonTokenType::NotSet, 0, ""};
        }

        JsonToken PeekToken()
        {
            if(CurrentToken.Type == JsonTokenType::NotSet)
            {
                CurrentToken = NextToken();
            }
        
            return CurrentToken;
        }

        JsonToken GetToken()
        {
            if(CurrentToken.Type == JsonTokenType::NotSet)
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
            return (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E';
        }

        static bool IsValidWhitespace(char c)
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        }

        static bool IsValidAfterLiteral(char c)
        {
            return c == '\0' || IsValidWhitespace(c) || c == ',' || c == ']' || c == '}';
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
                case '{': return {JsonTokenType::ObjectStart, TokenPosition, GetView()};
                case '}': return {JsonTokenType::ObjectEnd, TokenPosition, GetView()};
                case '[': return {JsonTokenType::ArrayStart, TokenPosition, GetView()};
                case ']': return {JsonTokenType::ArrayEnd, TokenPosition, GetView()};
                case ',': return {JsonTokenType::Comma, TokenPosition, GetView()};
                case ':': return {JsonTokenType::Colon, TokenPosition, GetView()};
                case 'n': return ParseNull();
                case '"': return ParseString();
                case 't' : case 'f': return ParseBoolean();
                
                default:
                {
                    if(IsValidNumberChar(Current))
                    {
                        return ParseNumber();
                    }
                }
            }

            return {JsonTokenType::Error, Position, "Invalid token"};
        }
        
        bool ProcessLiteral(std::string_view Literal)
        {
            const size_t CurrentPos = Position;
            const size_t Size = Literal.size();
            
            if(Input.substr(Position, Size) == Literal)
            {
                Position += Size;
                SkipWhitespace();
                
                if(IsValidAfterLiteral(Peek()))
                {
                    return true;
                }
            }
            
            Position = CurrentPos;
            return false;
        }

        JsonToken ParseNull()
        {
            JsonToken Token{JsonTokenType::Error, Position, ""};
            if(ProcessLiteral("null"))
            {
                Token.Value = "null";
                Token.Type = JsonTokenType::Null;
                return Token;
            }
            
            Token.Value = "Invalid null format [expected 'null']";
            return Token;
        }
    
        JsonToken ParseString()
        {
            const size_t TokenPosition = Position;
            Get(); // opening quote

            const size_t Start = Position;
            bool bEscaped = false;

#if SIMD_SUPPORTED
            const __m128i Quotes = _mm_set1_epi8('"');
            const __m128i Backslashes = _mm_set1_epi8('\\');
            const char* const Data = Input.data();

            while(Position + 16 <= Input.size())
            {
                const __m128i Chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(Data + Position));
                const __m128i IsQuote = _mm_cmpeq_epi8(Chunk, Quotes);
                const __m128i IsBackslash = _mm_cmpeq_epi8(Chunk, Backslashes);
                const unsigned Mask = _mm_movemask_epi8(_mm_or_si128(IsQuote, IsBackslash));

                if(Mask == 0)
                {
                    Position += 16;

                    continue;
                }

                Position += static_cast<size_t>(std::countr_zero(Mask));

                break;
            }
#endif

            while(Position < Input.size())
            {
                const char Current = Input[Position];

                if(Current == '\\')
                {
                    bEscaped = true;
                    Position += 2;

                    continue;
                }

                if(Current == '"') break;

                ++Position;
            }

            if(Position >= Input.size() || Input[Position] != '"')
            {
                Position = Start;

                return {JsonTokenType::Error, TokenPosition, "Invalid string format [missing closing quote]"};
            }

            std::string_view Value{Input.data() + Start, Position - Start};
            Position = Position + 1;

            SkipWhitespace();

            JsonToken Token{JsonTokenType::String, TokenPosition, Value};
            if(bEscaped)
            {
                Token.Type = JsonTokenType::StringEscaped;
            }

            return Token;
        }

        JsonToken ParseNumber()
        {
            const size_t Start = Position;
            
            auto ProcessDigits = [&]()
            {
                if(std::isdigit(static_cast<unsigned char>(Peek())))
                {
                    while(std::isdigit(static_cast<unsigned char>(Peek()))) Get();
                    return true;
                }
                return false;
            };
            
            if(Peek() == '-') //sign
            {
                Get();
            }
            
            if(std::isdigit(static_cast<unsigned char>(Peek())))
            {
                if(Peek() == '0' && std::isdigit(static_cast<unsigned char>(PeekAhead(1)))) //leading zeros are not allowed
                {
                    return {JsonTokenType::Error, Start, "Invalid number format [leading zeros are not allowed]"};
                }
                
                while(std::isdigit(static_cast<unsigned char>(Peek()))) Get();
            }
            else
            {
                return {JsonTokenType::Error, Start, "Invalid number format [expected digit as first character]"};
            }
            
            if(Peek() == '.') //decimal point
            {
                Get();
                if(!ProcessDigits())
                {
                    return {JsonTokenType::Error, Start, "Invalid number format [expected digit after decimal point]"};
                }
            }
            
            if(Peek() == 'e' || Peek() == 'E') //exponent
            {
                Get();
                if(Peek() == '+' || Peek() == '-')
                {
                    Get();
                }
                
                if(!ProcessDigits())
                {
                    return {JsonTokenType::Error, Start, "Invalid number format [expected digit in exponent]"};
                }
            }
            
            const size_t NumberEnd = Position;

            SkipWhitespace();
            if(!IsValidAfterLiteral(Peek()))
            {
                Position = NumberEnd;

                return {JsonTokenType::Error, Start, "Invalid number format [unexpected character]"};
            }

            return {JsonTokenType::Number, Start, Input.substr(Start, NumberEnd - Start)};
        }

        JsonToken ParseBoolean()
        {
            JsonToken Token{JsonTokenType::Error, Position, ""};
            
            if(ProcessLiteral("true"))
            {
                Token.Value = "true";
                Token.Type = JsonTokenType::Boolean;
                return Token;
            }
            else if(ProcessLiteral("false"))
            {
                Token.Value = "false";
                Token.Type = JsonTokenType::Boolean;
                return Token;
            }
            
            Token.Value = "Invalid boolean format [expected 'true' or 'false']";
            return Token;
        }
    
        void SkipWhitespace()
        {
            while (Position < Input.size() && IsValidWhitespace(Input[Position]))
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
        
        std::string_view GetView()
        {
            return Position < Input.size() ? std::string_view{&Input[Position++], 1} : std::string_view{};
        }
    
        size_t Position{};
        std::string_view Input{};
        JsonToken CurrentToken{};
    };
    
    namespace Internal
    {
        struct SizeVisitor
        {
            void operator()(std::string_view Str)
            {
                ToReserve += Str.size();
            }
                
            void operator()(size_t Size)
            {
                ToReserve += Size;
            }

            size_t size() const
            {
                return ToReserve;
            }

            size_t ToReserve{};
            static constexpr bool bSizeFastPath = true;
        };

        struct ResultVisitor
        {
            void operator()(std::string_view Str)
            {
                Result.append(Str);
            }

            size_t size() const
            {
                return Result.size();
            }

            std::string Result;
            static constexpr bool bSizeFastPath = false;
        };
    }
    
    class Json
    {
    public:
        static constexpr size_t MaxParseDepth = 1000;
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
                RootObject->Properties = DeepCopyMap(Other.RootObject->Properties);
            }
        }

        Json(Json&& Other) :
        Tokenizer{std::move(Other.Tokenizer)},
        CurrentToken{std::move(Other.CurrentToken)},
        ErrorMessage{std::move(Other.ErrorMessage)},
        RootObject{std::move(Other.RootObject)}
        {
            Other.Tokenizer.Init("");
            Other.CurrentToken = {JsonTokenType::NotSet, 0, ""};
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
                    RootObject->Properties = DeepCopyMap(Other.RootObject->Properties);
                }
                else
                {
                    RootObject.reset();
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
                Other.CurrentToken = {JsonTokenType::NotSet, 0, ""};
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
                ErrorMessage.reset();
            }

            auto& Value = RootObject->Properties[Key];
            return {Value};
        }

        JsonValueWrapper<const JsonValue> operator[](const std::string& Key) const
        {
            if(!RootObject) throw std::runtime_error("Root object is null, const access not possible");

            if(auto It = RootObject->Properties.find(Key); It != RootObject->Properties.end())
            {
                return {It->second};
            }

            static const JsonValue EmptyValue = UndefinedValue{};
            return {EmptyValue};
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
            CurrentToken = {JsonTokenType::NotSet, 0, ""};
        }

        void Parse(std::string_view Input)
        {
            Tokenizer.Init(Input);
            ErrorMessage.reset();

            RootObject = ParseObject(0);

            if(!HasError())
            {
                Peek();
                Consume();
                if(CurrentToken.Type != JsonTokenType::None)
                {
                    ThrowError(CurrentToken, "Unexpected content after root object");
                }
            }
        }

        [[nodiscard]] std::string Serialize(bool bPretty) const
        {
            Internal::ResultVisitor ResultVisitorInstance{};
            Internal::SizeVisitor SizeVisitorInstance{};

            if(HasError())
            {
                throw std::runtime_error("Cannot serialize a parser with an error state: " + GetError().value_or(""));
            }

            if(!RootObject) return ResultVisitorInstance.Result;
            SerializeObject(*RootObject, SizeVisitorInstance, bPretty, 0);

            ResultVisitorInstance.Result.reserve(SizeVisitorInstance.ToReserve);
            SerializeObject(*RootObject, ResultVisitorInstance, bPretty, 0);

            return std::move(ResultVisitorInstance.Result);
        }
        
        [[nodiscard]] bool HasError() const
        {
            return ErrorMessage.has_value();
        }

        [[nodiscard]] const std::optional<std::string>& GetError() const
        {
            if(ErrorMessage.has_value())
            {
                return ErrorMessage;
            }

            static const std::optional<std::string> NoError;
            return NoError;
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
            if(CurrentToken.Type == JsonTokenType::Error)
            {
                ThrowError(CurrentToken, CurrentToken.Value);
            }
        }

        //Serialization
        template<typename TVisitor>
        void SerializeValue(const JsonValue& Value, TVisitor&& Result, bool bPretty, size_t Depth) const;

        template<typename TVisitor>
        void SerializeArray(const JsonArray& Array, TVisitor&& Result, bool bPretty, size_t Depth) const;

        template<typename TVisitor>
        void SerializeObject(const JsonObject& Object, TVisitor&& Result, bool bPretty, size_t Depth) const;

        template<typename TVisitor>
        void SerializeString(std::string_view Text, TVisitor&& Result) const;

        template<typename TVisitor>
        void SerializeStringEscaped(const char* Data, size_t Size, TVisitor&& Result) const;

        template<typename TVisitor>
        void ApplyDepth(TVisitor&& Result, size_t Depth) const;
        

        //Deserialization
        bool DeserializeString(const JsonToken& Token, std::string& Result);
        bool DecodeStringEscapes(std::string_view Raw, std::string& Result);
        JsonValue ParseValue(size_t Depth);
        std::shared_ptr<JsonArray> ParseArray(size_t Depth);
        std::shared_ptr<JsonObject> ParseObject(size_t Depth);
        
        void ThrowError(const JsonToken& Token, std::string_view message);
    
    
        JsonTokenizer Tokenizer;
        JsonToken CurrentToken{};
        std::optional<std::string> ErrorMessage{}; 
        std::shared_ptr<JsonObject> RootObject;
    };
    
    
    template<typename TVisitor>
    void Json::SerializeValue(const JsonValue& Value, TVisitor&& Result, bool bPretty, size_t Depth) const
    {
        using TVisitorType = std::remove_reference_t<TVisitor>;
        static constexpr size_t IntFastPathSize = 20; // max size of int64_t in decimal representation
        static constexpr size_t DoubleFastPathSize = 32; // max size of double in decimal representation
        
        char Buffer[32];
        char* EndPtr = Buffer + sizeof(Buffer);

        if(HasType<int64_t>(Value))
        {
            if constexpr(TVisitorType::bSizeFastPath)
            {
                Result(IntFastPathSize);
            }
            else
            {
                const auto Res = std::to_chars(Buffer, EndPtr, std::get<int64_t>(Value));
                Result(std::string_view{Buffer, static_cast<uint64_t>(Res.ptr - Buffer)});
            }
        }
        else if(HasType<double>(Value))
        {
            if constexpr (TVisitorType::bSizeFastPath)
            {
                Result(DoubleFastPathSize);
            }
            else
            {
                const auto& Number = std::get<double>(Value);
                if(!std::isfinite(Number))
                {
                    Result("null");

                    return;
                }

                const auto Res = std::to_chars(Buffer, EndPtr, Number);
                Result(std::string_view{Buffer, static_cast<uint64_t>(Res.ptr - Buffer)});

                if(Number == std::floor(Number) && !std::memchr(Buffer, 'e', Res.ptr - Buffer) && !std::memchr(Buffer, 'E', Res.ptr - Buffer) && std::fabs(Number) < 1e16)
                {
                    Result(".0");
                }
            }
        }
        else if(HasType<nullptr_t>(Value) || HasType<UndefinedValue>(Value))
        {
           Result("null");
        }
        else if(HasType<bool>(Value))
        {
            Result(std::get<bool>(Value) ? "true" : "false");
        }
        else if(HasType<std::string>(Value))
        {
            const auto& String = std::get<std::string>(Value);
            const std::string_view Text{String.data(), String.size()};
            
            if constexpr(TVisitorType::bSizeFastPath)
            {
                Result(Text.size() + 4);
            }
            else
            {
                Result("\"");
                SerializeString(Text, Result);
                Result("\"");
            }
        }
        else if(HasType<JsonArray>(Value))
        {
            if(const auto& Ptr = std::get<std::shared_ptr<JsonArray>>(Value))
            {
                SerializeArray(*Ptr, Result, bPretty, Depth + 1);
            }
            else
            {
                Result("null");
            }
        }
        else if(HasType<JsonObject>(Value))
        {
            if(const auto& Ptr = std::get<std::shared_ptr<JsonObject>>(Value))
            {
                SerializeObject(*Ptr, Result, bPretty, Depth + 1);
            }
            else
            {
                Result("null");
            }
        }
    }

    template<typename TVisitor>
    void Json::SerializeString(std::string_view Text, TVisitor&& Result) const
    {
#if SIMD_SUPPORTED
        constexpr size_t SimdWidth = 16;
        const __m128i ControlLimit = _mm_set1_epi8(0x20);
        const __m128i Quotes = _mm_set1_epi8('"');
        const __m128i Backslashes = _mm_set1_epi8('\\');
        const char* const Data = Text.data();
        const size_t Size = Text.size();
        size_t CleanEnd{};

        while(CleanEnd + SimdWidth <= Size)
        {
            const __m128i Chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(Data + CleanEnd));
            const __m128i IsControl = _mm_cmplt_epi8(Chunk, ControlLimit);
            const __m128i IsQuote = _mm_cmpeq_epi8(Chunk, Quotes);
            const __m128i IsBackslash = _mm_cmpeq_epi8(Chunk, Backslashes);
            const __m128i Verdicts = _mm_or_si128(_mm_or_si128(IsControl, IsQuote), IsBackslash);

            const unsigned Mask = _mm_movemask_epi8(Verdicts);

            if(Mask != 0)
            {
                CleanEnd += static_cast<size_t>(std::countr_zero(Mask));

                break;
            }

            CleanEnd += SimdWidth;
        }

        Result(std::string_view{Data, CleanEnd});

        SerializeStringEscaped(Data + CleanEnd, Size - CleanEnd, Result);
#else
        SerializeStringEscaped(Text.data(), Text.size(), Result);
#endif
    }

    template<typename TVisitor>
    void Json::SerializeStringEscaped(const char* Data, size_t Size, TVisitor&& Result) const
    {
        size_t Offset{};

        while(Offset < Size)
        {
            const char Current = Data[Offset];

            if(static_cast<unsigned char>(Current) >= 0x20 && Current != '"' && Current != '\\')
            {
                const size_t Start = Offset;

                do
                {
                    ++Offset;
                }
                while(Offset < Size && static_cast<unsigned char>(Data[Offset]) >= 0x20 && Data[Offset] != '"' && Data[Offset] != '\\');

                Result(std::string_view{Data + Start, Offset - Start});

                continue;
            }

            switch(Current)
            {
                case '"':
                {
                    Result("\\\"");
                    break;
                }
                case '\\':
                {
                    Result("\\\\");
                    break;
                }
                case '\b':
                {
                    Result("\\b");
                    break;
                }
                case '\f':
                {
                    Result("\\f");
                    break;
                }
                case '\n':
                {
                    Result("\\n");
                    break;
                }
                case '\r':
                {
                    Result("\\r");
                    break;
                }
                case '\t':
                {
                    Result("\\t");
                    break;
                }
                default:
                {
                    static constexpr char HexDigits[] = "0123456789abcdef";

                    char Escape[6];
                    Escape[0] = '\\';
                    Escape[1] = 'u';
                    Escape[2] = '0';
                    Escape[3] = '0';
                    Escape[4] = HexDigits[(Current >> 4) & 0xF];
                    Escape[5] = HexDigits[Current & 0xF];
                    Result(std::string_view{Escape, 6});
                    break;
                }
            }

            ++Offset;
        }
    }

    template<typename TVisitor>
    void Json::SerializeArray(const JsonArray& Array, TVisitor&& Result, bool bPretty, size_t Depth) const
    {
        Result("[");

        size_t Written{};
        for(const auto& Value : Array.Values)
        {
            if(Written > 0)
            {
                Result(",");
            }

            if(bPretty)
            {
                ApplyDepth(Result, Depth + 1);
            }
            
            SerializeValue(Value, Result, bPretty, Depth);
            ++Written;
        }

        if(bPretty && Written > 0)
        {
            ApplyDepth(Result, Depth);
        }

        Result("]");
    }

    template<typename TVisitor>
    void Json::SerializeObject(const JsonObject& Object, TVisitor&& Result, bool bPretty, size_t Depth) const
    {
        using TVisitorType = std::remove_reference_t<TVisitor>;
        Result("{");
        
        std::string_view Separator = bPretty ? "\": " : "\":";
        
        size_t Written{};
        for(const auto& [Key, Value] : Object.Properties)
        {
            if(Written > 0)
            {
                Result(",");
            }
            
            bool bNeedsObjectSeparator = false;
            if(bPretty)
            {
                ApplyDepth(Result, Depth + 1);
                if(HasType<JsonObject>(Value) || HasType<JsonArray>(Value))
                {
                   bNeedsObjectSeparator = true;
                }
            }
            
            if constexpr(TVisitorType::bSizeFastPath)
            {
                Result(Key.size() + 4);
            }
            else
            {
                Result("\"");
                SerializeString(Key, Result);
                Result(Separator);
            }
            
            if(bNeedsObjectSeparator)
            {
                ApplyDepth(Result, Depth + 1);
            }
            
            SerializeValue(Value, Result, bPretty, Depth);
            ++Written;
        }

        if(bPretty && Written > 0)
        {
            ApplyDepth(Result, Depth);
        }

        Result("}");
    }

    template<typename TVisitor>
    void Json::ApplyDepth(TVisitor&& Result, size_t Depth) const
    {
        using TVisitorType = std::remove_reference_t<TVisitor>;
        if constexpr(TVisitorType::bSizeFastPath)
        {
            Result(Depth + 1);
        }
        else
        {
            Result("\n");
            for(size_t i = 0; i < Depth; ++i)
            {
                Result("\t");
            }
        }
    }

    inline bool Json::DeserializeString(const JsonToken& Token, std::string& Result)
    {
        if(Token.Type == JsonTokenType::String)
        {
            Result.assign(Token.Value);

            return true;
        }

        Result.clear();
        return DecodeStringEscapes(Token.Value, Result);
    }

    inline bool Json::DecodeStringEscapes(std::string_view Raw, std::string& Result)
    {
        auto AppendCodePoint = [&](std::uint32_t CodePoint)
        {
            if(CodePoint <= 0x7F)
            {
                Result.push_back(static_cast<char>(CodePoint));
            }
            else if(CodePoint <= 0x7FF)
            {
                Result.push_back(static_cast<char>(0xC0 | (CodePoint >> 6)));
                Result.push_back(static_cast<char>(0x80 | (CodePoint & 0x3F)));
            }
            else if(CodePoint <= 0xFFFF)
            {
                Result.push_back(static_cast<char>(0xE0 | (CodePoint >> 12)));
                Result.push_back(static_cast<char>(0x80 | ((CodePoint >> 6) & 0x3F)));
                Result.push_back(static_cast<char>(0x80 | (CodePoint & 0x3F)));
            }
            else
            {
                Result.push_back(static_cast<char>(0xF0 | (CodePoint >> 18)));
                Result.push_back(static_cast<char>(0x80 | ((CodePoint >> 12) & 0x3F)));
                Result.push_back(static_cast<char>(0x80 | ((CodePoint >> 6) & 0x3F)));
                Result.push_back(static_cast<char>(0x80 | (CodePoint & 0x3F)));
            }
        };

        auto ReadHex = [&](size_t Offset, std::uint32_t& ValueOut) -> bool
        {
            if(Offset + 4 > Raw.size())
            {
                return false;
            }

            ValueOut = 0;
            for(size_t a = 0; a < 4; a++)
            {
                const char HexChar = Raw[Offset + a];
                std::uint32_t Digit;

                if(HexChar >= '0' && HexChar <= '9')
                {
                    Digit = static_cast<std::uint32_t>(HexChar - '0');
                }
                else if(HexChar >= 'a' && HexChar <= 'f')
                {
                    Digit = static_cast<std::uint32_t>(HexChar - 'a' + 10);
                }
                else if(HexChar >= 'A' && HexChar <= 'F')
                {
                    Digit = static_cast<std::uint32_t>(HexChar - 'A' + 10);
                }
                else
                {
                    return false;
                }

                ValueOut = (ValueOut << 4) | Digit;
            }

            return true;
        };

        size_t a = 0;

        while(a < Raw.size())
        {
#if SIMD_SUPPORTED
            const __m128i Backslashes = _mm_set1_epi8('\\');

            if(Raw[a] != '\\')
            {
                size_t Scan = a;
                size_t RunEnd = Scan;

                while(Scan + 16 <= Raw.size())
                {
                    const __m128i Chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(Raw.data() + Scan));
                    const __m128i IsBackslash = _mm_cmpeq_epi8(Chunk, Backslashes);
                    const unsigned Mask = _mm_movemask_epi8(IsBackslash);

                    if(Mask != 0)
                    {
                        RunEnd = Scan + static_cast<size_t>(std::countr_zero(Mask));

                        break;
                    }

                    Scan += 16;
                    RunEnd = Scan;
                }

                while(RunEnd < Raw.size() && Raw[RunEnd] != '\\')
                {
                    ++RunEnd;
                }

                Result.append(Raw.data() + a, RunEnd - a);
                a = RunEnd;

                continue;
            }
#else
            if(Raw[a] != '\\')
            {
                Result.push_back(Raw[a]);
                ++a;

                continue;
            }
#endif

            if(++a >= Raw.size())
            {
                return false;
            }

            switch(Raw[a])
            {
                case '"':
                {
                    Result.push_back('"');
                    break;
                }
                case '\\':
                {
                    Result.push_back('\\');

                    break;
                }
                case '/':
                {
                    Result.push_back('/');

                    break;
                }
                case 'b':
                {
                    Result.push_back('\b');

                    break;
                }
                case 'f':
                {
                    Result.push_back('\f');

                    break;
                }
                case 'n':
                {
                    Result.push_back('\n');

                    break;
                }
                case 'r':
                {
                    Result.push_back('\r');

                    break;
                }
                case 't':
                {
                    Result.push_back('\t');

                    break;
                }
                case 'u':
                {
                    std::uint32_t CodePoint;
                    if(!ReadHex(a + 1, CodePoint))
                    {
                        return false;
                    }
                    a += 4;

                    if(CodePoint >= 0xD800 && CodePoint <= 0xDBFF)
                    {
                        if(a + 6 < Raw.size() && Raw[a + 1] == '\\' && Raw[a + 2] == 'u')
                        {
                            std::uint32_t Low;
                            if(ReadHex(a + 3, Low) && Low >= 0xDC00 && Low <= 0xDFFF)
                            {
                                a += 6;
                                AppendCodePoint(0x10000 + ((CodePoint - 0xD800) << 10) + (Low - 0xDC00));

                                ++a;

                                continue;
                            }
                            else
                            {
                                return false;
                            }
                        }
                        else
                        {
                            return false;
                        }
                    }

                    if(CodePoint >= 0xDC00 && CodePoint <= 0xDFFF)
                    {
                        return false;
                    }

                    AppendCodePoint(CodePoint);

                    break;
                }
                default:
                {
                    return false;
                }
            }

            ++a;
        }

        return true;
    }

    inline JsonValue Json::ParseValue(size_t Depth)
    {
        if(Depth > MaxParseDepth)
        {
            ThrowError(CurrentToken, "JSON nesting depth limit exceeded");
            return {};
        }

        Peek();
        switch(CurrentToken.Type)
        {
            case JsonTokenType::ObjectStart:
            {
                return ParseObject(Depth);
            }
            case JsonTokenType::ArrayStart:
            {
                return ParseArray(Depth);
            }
            case JsonTokenType::String:
            case JsonTokenType::StringEscaped:
            {
                Consume();

                std::string Result;
                if(!DeserializeString(CurrentToken, Result))
                {
                    ThrowParserError(CurrentToken, "Invalid string format [bad escape sequence or lone surrogate]");
                }

                return std::move(Result);
            }
            case JsonTokenType::Number:
            {
                const std::string_view NumberText = CurrentToken.Value;
                const char* NumEnd = NumberText.data() + NumberText.size();

                int64_t IntResult = 0;
                const auto IntPtr = std::from_chars(NumberText.data(), NumEnd, IntResult);
                if(IntPtr.ptr == NumEnd && IntPtr.ec == std::errc{})
                {
                    Consume();

                    return IntResult;
                }

                double FloatResult = 0.0;
                const auto FloatPtr = std::from_chars(NumberText.data(), NumEnd, FloatResult);
                if(FloatPtr.ec == std::errc{} || (FloatPtr.ec == std::errc::result_out_of_range && std::isfinite(FloatResult)))
                {
                    Consume();

                    return FloatResult;
                }

                Consume();
                ThrowParserError(CurrentToken, std::format("Invalid number format [cannot parse '{}']", NumberText));
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

        ThrowParserError(CurrentToken, std::format("Unexpected token while parsing value: {}", CurrentToken.Value));
    }

    inline std::shared_ptr<JsonArray> Json::ParseArray(size_t Depth)
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
                auto Value = ParseValue(Depth + 1);
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

    inline std::shared_ptr<JsonObject> Json::ParseObject(size_t Depth)
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
            if(CurrentToken.Type != JsonTokenType::String && CurrentToken.Type != JsonTokenType::StringEscaped)
            {
                ThrowParserError(CurrentToken, "Expected string key");
            }

            // Get the key
            Consume();
            auto Key = CurrentToken.Value;
            
            std::string DecodedKey;
            if(CurrentToken.Type == JsonTokenType::StringEscaped)
            {
                if(!DeserializeString(CurrentToken, DecodedKey))
                {
                    ThrowParserError(CurrentToken, "Invalid string format [bad escape sequence or lone surrogate]");
                }

                Key = DecodedKey;
            }

            // Get the colon
            Consume();
            if(CurrentToken.Type != JsonTokenType::Colon)
            {
                ThrowParserError(CurrentToken, "Expected ':'");
            }

            // Parse the value
            auto Value = ParseValue(Depth + 1);
            if(HasError()) return {};
                
            Result->Properties.insert_or_assign(std::string{std::move(Key)}, std::move(Value));

            Consume();
            if(CurrentToken.Type != JsonTokenType::Comma && CurrentToken.Type != JsonTokenType::ObjectEnd)
            {
                ThrowParserError(CurrentToken, "Expected ',' or '}'");
            }

            if(CurrentToken.Type == JsonTokenType::ObjectEnd) break;
        }

        return Result;
    }

    inline void Json::ThrowError(const JsonToken& Token, std::string_view message)
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
        
        const std::string TokenValue = Token.Type == JsonTokenType::Error ? "Tokenization Error" : std::string{Token.Value};
        ErrorMessage = std::format("Error at position {}[{}]: {} \nError Reason: {}", Token.Position, TokenValue, ErrorLocation, message);
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

    inline JsonValue DeepCopyValue(const JsonValue& Source)
    {
        if(auto* Ptr = std::get_if<std::shared_ptr<JsonArray>>(&Source); Ptr && *Ptr)
        {
            auto Copy = std::make_shared<JsonArray>(**Ptr);
            for(auto& Value : Copy->Values)
            {
                Value = DeepCopyValue(Value);
            }

            return Copy;
        }

        if(auto* Ptr = std::get_if<std::shared_ptr<JsonObject>>(&Source); Ptr && *Ptr)
        {
            auto Copy = std::make_shared<JsonObject>(**Ptr);
            for(auto& [Key, Value] : Copy->Properties)
            {
                Value = DeepCopyValue(Value);
            }

            return Copy;
        }

        return Source;
    }

    inline std::unordered_map<std::string, JsonValue> DeepCopyMap(const std::unordered_map<std::string, JsonValue>& Source)
    {
        std::unordered_map<std::string, JsonValue> Copy;
        Copy.reserve(Source.size());
        for(const auto& [Key, Value] : Source)
        {
            Copy.emplace(Key, DeepCopyValue(Value));
        }
        return Copy;
    }

    inline std::vector<JsonValue> DeepCopyArray(const std::vector<JsonValue>& Source)
    {
        std::vector<JsonValue> Copy;
        Copy.reserve(Source.size());
        for(const auto& Value : Source)
        {
            Copy.push_back(DeepCopyValue(Value));
        }
        return Copy;
    }
}

#undef ThrowParserError
#undef SIMD_SUPPORTED