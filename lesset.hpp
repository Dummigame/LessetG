
#ifdef LESSET
    #error "Only include lesset.hpp once."
#endif

#define LESSET

#include <cctype>
#include <random>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <boost/math/constants/constants.hpp>
#include <sstream>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/math/ccmath/fmod.hpp>
#include <type_traits>
#include <unordered_map>
#include <thread>
#include <future>

namespace lessetB
{
bool isValidInput(char);

using boost::multiprecision::cpp_dec_float_100;

inline std::random_device randev;
inline std::mt19937 randomMt(randev());

#define MAXOUTPUTPRECISION 100

#define MAXKEYWORDLENGTH 6 // Change this when adding long keywords

enum drawPos
{
    ZERO,
    LEFT,
    RIGHT,
};

enum pass
{
    SUBEXPRESSIONS,
    UNARYOPS,
    EXPONENTIATION,
    FUNCTIONS,
    UNARYMINUS,
    MULTIPLICATIONIMPLICIT,
    MULTIPLICATION,
    ADDITION,
    COMPARISONS,
    LOGICALS
};

enum class token_t
{
    BINARYOP,
    UNARYOP,
    FUNCTION,
    IF,
    NUMBER,
    ROOT,
    LOG,
    DERIVE,
    MEAN, // Meanie
    MEDIAN,
    STDEVP,
    GCF,
    LCM,
    RNDINT,
    RNDSEL,
    ABS,
    MAX,
    SMAX,
    SMIN,
    SABS,
    MIX,
    MIN,
    SUBEXPR,
    VARIABLE,
    CONSTANT,
    ASSIGNMENTVARIABLE,
    ASSIGNMENTMACRO,
    INVALID
};

enum class tokenCategory_t
{
    NUMBER,
    FUNCTION,
    SUBEXPR,
    OPERATOR,
    ASSIGNMENT,
    INVALID
};

bool isNumberPart(char input);

bool isNumber(const std::string &input);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Point
{
    const float x{};
    const float y{};
    Point(float inX, float inY) : x(inX), y(inY){}
};

struct Options
{
    bool graph{};
    cpp_dec_float_100 xMin{};
    cpp_dec_float_100 xMax{};  
    cpp_dec_float_100 xStep{}; // Hey, reference
    cpp_dec_float_100 aroundTruthinessLeniency{0.01};
    bool interpolateDiscontinuities{};
    bool followImplicitMultiplicationPriorityConvention{true};
    bool showFractions{true};
    std::string ans;
};

struct Variable
{
    Variable(std::string inName, std::string inValue) : name(inName), value(inValue){}
    std::string name;
    std::string value;
};

struct Macro
{
    Macro(std::string inName, std::string inValue) : name(inName), value(inValue){}
    std::string name;
    std::string value;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool sortVariablesByNameLength(Variable name1, Variable name2)
{
    return name1.name.length()>name2.name.length();
}

inline bool sortMacroesByNameLength(Macro name1, Macro name2)
{
    return name1.name.length()>name2.name.length();
}

class Token;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace globals
{
    inline Options aroundLeniency;
    inline std::vector<Variable> userVariables;
    inline std::vector<Macro> userMacros;
    inline std::pair<std::vector<double>,std::vector<double>> points; 
    inline std::string previousResult;

    inline std::unordered_map<std::string, std::vector<Token>> tokenMemory;
    
    const std::unordered_map<std::string, token_t> symbols
    {
        {"+",    token_t::BINARYOP},
        {"*",    token_t::BINARYOP},
        {"/",    token_t::BINARYOP},
        {"^",    token_t::BINARYOP},
        {"%",    token_t::BINARYOP},
        {"<",    token_t::BINARYOP},
        {">",    token_t::BINARYOP},
        {"=",    token_t::BINARYOP},
        {"mod",    token_t::BINARYOP},
        {"nPk",    token_t::BINARYOP},
        {"nCk",    token_t::BINARYOP},
        {"**",    token_t::BINARYOP},
        {"AND",    token_t::BINARYOP},
        {"XOR",    token_t::BINARYOP},
        {"AROUND",    token_t::BINARYOP},
        {"NOR",    token_t::BINARYOP},
        {"OR",    token_t::BINARYOP},
        {"=!",    token_t::BINARYOP},
        {">=",    token_t::BINARYOP},
        {"<=",    token_t::BINARYOP},

        {"!", token_t::UNARYOP},
        {"-", token_t::UNARYOP},
        {"!!", token_t::UNARYOP},

        {"pi", token_t::CONSTANT },
        {"e", token_t::CONSTANT },
        {"a", token_t::CONSTANT },
        {"rnd", token_t::CONSTANT },
        {"rndint", token_t::CONSTANT },
        {"ec", token_t::CONSTANT },
        {"c", token_t::CONSTANT },
        {"R", token_t::CONSTANT },
        {"G", token_t::CONSTANT },
        {"g", token_t::CONSTANT },
        {"o", token_t::CONSTANT },
        {"h", token_t::CONSTANT },
        {"k", token_t::CONSTANT },
        {"H0", token_t::CONSTANT },
        {"Z0", token_t::CONSTANT },
        {"U0", token_t::CONSTANT },
        {"E0", token_t::CONSTANT },
        {"tau", token_t::CONSTANT },
        {"phi", token_t::CONSTANT },
        {"eul", token_t::CONSTANT },
        {"rad", token_t::CONSTANT },
        {"dgr", token_t::CONSTANT },
        {"inf", token_t::CONSTANT },
        {"ppm", token_t::CONSTANT },
        {"ppb", token_t::CONSTANT },
        {"ppt", token_t::CONSTANT },
        {"prc", token_t::CONSTANT },
        {"me", token_t::CONSTANT },
        {"ma", token_t::CONSTANT },
        {"Na", token_t::CONSTANT },
        {"true", token_t::CONSTANT },
        {"false", token_t::CONSTANT },
        {"ans", token_t::CONSTANT},

        {"sinc", token_t::FUNCTION},
        {"exp", token_t::FUNCTION},
        {"sign", token_t::FUNCTION},
        {"sqrt", token_t::FUNCTION},
        {"cbrt", token_t::FUNCTION},
        {"qtrt", token_t::FUNCTION},

        {"sin", token_t::FUNCTION},
        {"cos", token_t::FUNCTION},
        {"tan", token_t::FUNCTION},
        {"sinh", token_t::FUNCTION},
        {"cosh", token_t::FUNCTION},
        {"tanh", token_t::FUNCTION},

        {"asin", token_t::FUNCTION},
        {"acos", token_t::FUNCTION},
        {"atan", token_t::FUNCTION},
        {"asinh", token_t::FUNCTION},
        {"acosh", token_t::FUNCTION},
        {"atanh", token_t::FUNCTION},

        {"sec", token_t::FUNCTION},
        {"csc", token_t::FUNCTION},
        {"cot", token_t::FUNCTION},
        {"sech", token_t::FUNCTION},
        {"csch", token_t::FUNCTION},
        {"coth", token_t::FUNCTION},

        {"asec", token_t::FUNCTION},
        {"acsc", token_t::FUNCTION},
        {"acot", token_t::FUNCTION},
        {"asech", token_t::FUNCTION},
        {"acsch", token_t::FUNCTION}, //aschhschhshuhuschush
        {"acoth", token_t::FUNCTION},

        {"ln", token_t::FUNCTION},
        {"abs", token_t::FUNCTION},
        {"floor", token_t::FUNCTION},
        {"ceil", token_t::FUNCTION},
        {"round", token_t::FUNCTION},
        {"sat", token_t::FUNCTION},
        {"ReLU", token_t::FUNCTION},
        {"sstep", token_t::FUNCTION},

        {"x", token_t::VARIABLE},

    };

    const std::unordered_map<std::string, token_t> unaryOperations
    {
        {"!", token_t::UNARYOP},
        {"-", token_t::UNARYOP},

        {"!!", token_t::UNARYOP},
    };

    const std::unordered_map<std::string, token_t> binaryOperations
    {
        {"+",    token_t::BINARYOP},
        {"*",    token_t::BINARYOP},
        {"/",    token_t::BINARYOP},
        {"^",    token_t::BINARYOP},
        {"%",    token_t::BINARYOP},
        {"<",    token_t::BINARYOP},
        {">",    token_t::BINARYOP},
        {"=",    token_t::BINARYOP},

        {"mod",    token_t::BINARYOP},
        {"nPk",    token_t::BINARYOP},
        {"nCk",    token_t::BINARYOP},
        {"**",    token_t::BINARYOP},
        {"AND",    token_t::BINARYOP},
        {"XOR",    token_t::BINARYOP},
        {"AROUND",    token_t::BINARYOP},
        {"NOR",    token_t::BINARYOP},
        {"OR",    token_t::BINARYOP},
        {"=!",    token_t::BINARYOP},
        {">=",    token_t::BINARYOP},
        {"<=",    token_t::BINARYOP},
    };

    const std::unordered_map<std::string, token_t> functions
    {
        {"sinc", token_t::FUNCTION},
        {"exp", token_t::FUNCTION},
        {"sign", token_t::FUNCTION},
        {"sqrt", token_t::FUNCTION},
        {"cbrt", token_t::FUNCTION},
        {"qtrt", token_t::FUNCTION},

        {"sin", token_t::FUNCTION},
        {"cos", token_t::FUNCTION},
        {"tan", token_t::FUNCTION},
        {"sinh", token_t::FUNCTION},
        {"cosh", token_t::FUNCTION},
        {"tanh", token_t::FUNCTION},

        {"asin", token_t::FUNCTION},
        {"acos", token_t::FUNCTION},
        {"atan", token_t::FUNCTION},
        {"asinh", token_t::FUNCTION},
        {"acosh", token_t::FUNCTION},
        {"atanh", token_t::FUNCTION},

        {"sec", token_t::FUNCTION},
        {"csc", token_t::FUNCTION},
        {"cot", token_t::FUNCTION},
        {"sech", token_t::FUNCTION},
        {"csch", token_t::FUNCTION},
        {"coth", token_t::FUNCTION},

        {"asec", token_t::FUNCTION},
        {"acsc", token_t::FUNCTION},
        {"acot", token_t::FUNCTION},
        {"asech", token_t::FUNCTION},
        {"acsch", token_t::FUNCTION}, //aschhschhshuhuschush
        {"acoth", token_t::FUNCTION},

        {"ln", token_t::FUNCTION},
        {"abs", token_t::FUNCTION},
        {"floor", token_t::FUNCTION},
        {"ceil", token_t::FUNCTION},
        {"round", token_t::FUNCTION},
        {"sat", token_t::FUNCTION},
        {"ReLU", token_t::FUNCTION},
        {"sstep", token_t::FUNCTION},
    };

    const std::unordered_map<std::string, token_t> multiArgFunctions
    {
        {"root", token_t::ROOT},
        {"log", token_t::LOG},
        {"derive", token_t::DERIVE},
        {"mean", token_t::MEAN},
        {"median", token_t::MEDIAN},
        {"stdevp", token_t::STDEVP},
        {"gcf", token_t::GCF},
        {"gcd", token_t::GCF},
        {"hcf", token_t::GCF},
        {"hcd", token_t::GCF},
        {"lcm", token_t::LCM},
        {"rndint", token_t::RNDINT},
        {"rndsel", token_t::RNDSEL},
        {"max", token_t::MAX},
        {"smax", token_t::SMAX},
        {"min", token_t::MIN},
        {"smin", token_t::SMIN},
        {"sabs", token_t::SABS},
        {"mix", token_t::MIX},
        {"if", token_t::IF},
    };

    const std::unordered_map<std::string, std::string> constants
    {
        {"e" , "2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427"},
        {"pi" , "3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068"},
        {"tau" , "6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234136"},
        {"phi" , "1.618033988749894848204586834365638117720309179805762862135448622705260462818902449707207204189391137"},
        {"eul" , "0.5772156649015328606065120900824024310421593359399235988057672348848677267776646709369470632917467495"},
        {"rad" , "57.29577951308232087679815481410517033240547246656432154916024386120284714832155263244096899585111094"},
        {"dgr" , "0.01745329251994329576923690768488612713442871888541725456097191440171009114603449443682241569634509482"},
        {"ppm" , "0.000001"},
        {"ppb" , "0.000000001"},
        {"ppt" , "0.000000000001"},
        {"prc" , "0.01"},
        {"c" , "299792458"},
        {"G" , "6.6743e-11"},
        {"g" , "9.80665"},
        {"o" , "5.670374419e-08"},
        {"k" , "1.380649e-23"},
        {"a" , "0.0072973525693"},
        {"h" , "6.62607015e-34"},
        {"inf" , "inf"},
        {"true" , "1"},
        {"false" , "0"},
        {"H0" , "2.2e-18"},
        {"me" , "5.9722e+24"},
        {"ec" , "1.602176634e-19"},
        {"Z0" , "376.730313668"},
        {"U0" , "1.25663706212e-06"},
        {"E0" , "8.8541878128e-12"},
        {"ma" , "1.6605390666e-27"},
        {"R" , "8.31446261815"},
        {"Na" , "6.02214076e+23"},
        {"ans" , "ans"},
        {"rnd", "rnd"}, // These are replaced later
        {"rndint","rndint"},
    };

    const std::unordered_map<std::string, std::string> valueToConstants
    {
        { "2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427","ℯ"},
        { "3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068","π"},
        {"6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234136","τ"},
        { "1.618033988749894848204586834365638117720309179805762862135448622705260462818902449707207204189391137","φ"},
        { "0.5772156649015328606065120900824024310421593359399235988057672348848677267776646709369470632917467495","γ"},
        { "57.29577951308232087679815481410517033240547246656432154916024386120284714832155263244096899585111094","rad"},
        {"0.01745329251994329576923690768488612713442871888541725456097191440171009114603449443682241569634509482","dgr" },
        { "299792458","c"},
        { "6.6743e-11","G"},
        { "9.80665","g"},
        {"5.670374419e-08","o" },
        {"1.380649e-23","k" },
        {"0.0072973525693","a" },
        {"6.62607015e-34","h" },
        {"inf","∞" },
        {"-inf","-∞" },
        {"2.2e-18","H0" },
        {"5.9722e+24","me" },
        {"1.602176634e-19","ec" },
        {"376.730313668","Z0" },
        {"1.25663706212e-06","U0" },
        {"8.8541878128e-12","E0" },
        {"1.6605390666e-27","ma" },
        {"8.31446261815","R" },
        {"6.02214076e+23","Na" },
    };

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Token
{

    private:

    token_t tokenType{};
    tokenCategory_t tokenCategory{};
    std::string tokenValue{};

    ///////////////////////////////////////////////
    token_t determineType(std::string &value)
    {
        if(isNumber(value)) return token_t::NUMBER;

        for(size_t i{}; i<globals::userVariables.size(); i++)
        {
            if(value==globals::userVariables.at(i).name) return token_t::CONSTANT;
        }

        if(value=="h*") return token_t::BINARYOP;

        else if(isSubexpr(value)) return token_t::SUBEXPR;
        else if(isAbs(value)) return token_t::ABS;
        else if(isAssignment(value)!=token_t::INVALID) return isAssignment(value);
        token_t tokenTypeCandidate = isMultiArgFunction(value);

        if(tokenTypeCandidate==token_t::INVALID)
        {
            std::string candidate;
            for(size_t i{MAXKEYWORDLENGTH}; i>0; i--)
            {
                candidate=value.substr(0,i);
                if(globals::symbols.find(candidate)!=globals::symbols.end())
                {
                    return globals::symbols.find(candidate)->second;
                }
            }
        }
        return tokenTypeCandidate;
    }
    
    static token_t isAssignment(const std::string &input)
    {
        if((input.find("let")==0) && input.find('=')!=std::string::npos) return token_t::ASSIGNMENTVARIABLE;
        else if(input.find("set")==0 && input.find('=')!=std::string::npos) return token_t::ASSIGNMENTMACRO;

        else return token_t::INVALID;
    }

    ///////////////////////////////////////////////
    token_t isMultiArgFunction(std::string &input)
    {
        size_t offset{};
        token_t type{token_t::INVALID};

        for(size_t i{MAXKEYWORDLENGTH}; i>0; i--)
        {
            if(globals::multiArgFunctions.find(input.substr(0,i))!=globals::multiArgFunctions.end())
            {
                type=globals::multiArgFunctions.find(input.substr(0,i))->second;
                offset=globals::multiArgFunctions.find(input.substr(0,i))->first.length()+1;
                break;
            }
        }
        if(type==token_t::INVALID) return type;
        for(size_t i{offset}; i<input.length(); i++)
        {
            tokenValue.push_back(input.at(i));
        }
        return type;
    }
    ///////////////////////////////////////////////
    bool isAbs(std::string &input)
    {
        if((input.at(0)!='|' || input.at(input.length()-1)!='|')&&input.find("abs(")!=0) return false;
        
        if(input.at(0)=='|') for(size_t i{1}; i<input.length()-1; i++) tokenValue.push_back(input.at(i));
        else for(size_t i{4}; i<input.length(); i++) tokenValue.push_back(input.at(i));
        return true;
    }    
    ///////////////////////////////////////////////
    static bool isSubexpr(std::string &input)
    {
        if(input.find(')')!=std::string::npos && input.length()<2) return false;
        if(input.at(0)=='(')
        {
            input.erase(0, 1);
            return true;
        }
        return false;
    }
    ///////////////////////////////////////////////
    static std::string replaceConstants(std::string &input)
    {

        if(globals::constants.find(input)!=globals::constants.end()) return globals::constants.find(input)->second;

        for(size_t i{}; i<globals::userVariables.size(); i++)
        {
            if(input==globals::userVariables.at(i).name) return globals::userVariables.at(i).value;
        }

        return "0";
    }
    ///////////////////////////////////////////////
    static tokenCategory_t determineTokenCategory(token_t &type)
    {
        if(type==token_t::NUMBER || type==token_t::VARIABLE || type==token_t::CONSTANT) return tokenCategory_t::NUMBER;

        else if(type==token_t::SUBEXPR ||
                type==token_t::ROOT || type==token_t::ABS || type==token_t::MAX || type==token_t::SMAX || type==token_t::SMIN || type==token_t::SABS || type==token_t::IF ||
                type==token_t::MIN || type==token_t::MEDIAN || type==token_t::STDEVP || type==token_t::GCF || type==token_t::LCM || type==token_t::DERIVE || type==token_t::MIX ||
                type==token_t::LOG || type==token_t::MEAN || type==token_t::RNDINT || type==token_t::RNDSEL) return tokenCategory_t::SUBEXPR;

        else if(type==token_t::FUNCTION) return tokenCategory_t::FUNCTION;

        else if(type==token_t::ASSIGNMENTVARIABLE || type==token_t::ASSIGNMENTMACRO) return tokenCategory_t::ASSIGNMENT;

        else if(type==token_t::INVALID) return tokenCategory_t::INVALID;

        else return tokenCategory_t::OPERATOR;
    }
    ///////////////////////////////////////////////
    ///////////////////////////////////////////////

    public:

    Token(std::string value)
    {
        tokenType = determineType(value);
        if(tokenType==token_t::CONSTANT) tokenValue=replaceConstants(value);
        
        tokenCategory=determineTokenCategory(tokenType);
        if(tokenValue=="")tokenValue = value;
    }
    ///////////////////////////////////////////////
    template <typename T>
    T number(T xValue=NAN)
    {
        if(xValue!=NAN && this->tokenType==token_t::VARIABLE)
        {
            std::ostringstream asOSStream;
            if constexpr(std::is_same<T,cpp_dec_float_100>()) asOSStream.precision(MAXOUTPUTPRECISION);
            else if constexpr(std::is_same<T, long double>()) asOSStream.precision(17);
            else if constexpr(std::is_same<T, double>()) asOSStream.precision(15);
            else if constexpr(std::is_same<T, float>()) asOSStream.precision(6);
            // else die
            asOSStream << xValue;
            this->tokenValue=asOSStream.str();
            this->tokenType=token_t::NUMBER;
        }

        if(tokenValue=="rnd" || tokenValue=="rndint") return NAN;

        if (tokenType != token_t::NUMBER && tokenType != token_t::CONSTANT) return NAN;
        if constexpr(std::is_same<T,cpp_dec_float_100>()) return static_cast<cpp_dec_float_100>(tokenValue);
        else return std::stold(tokenValue);
    }
    ///////////////////////////////////////////////
    std::string value()
    {
        return tokenValue;
    }  
    token_t type()
    {
        return tokenType;
    }
    tokenCategory_t typeCategory()
    {
        return tokenCategory;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<Token> getTokens(const std::string&, bool resetFirstRun=false);
void parseMultiArgFunction(const std::string &input, std::vector<Token> &tokens, const char* functionName, size_t &i, bool &inFunctionCall, size_t argCount=SIZE_MAX);
void getVariableArgs(std::vector<Token>&, Options&);

template <typename T = cpp_dec_float_100> T calculation(std::vector<Token>, const T xValue);
std::vector<Point> calculationCaller(std::vector<Token> &tokens, double xValue, double xValueMax, size_t threadNumber);

template <typename T = cpp_dec_float_100> T evaluateAbs(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateIf(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateLog(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateRoot(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateUnary(Token&, Token&, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateBinary(Token&, Token&, Token&, const T xValue);

template <typename T = cpp_dec_float_100> T evaluateMean(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateMedian(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateStdevp(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateRndsel(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateMax(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateLeast(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateGcf(Token &arg, const T xValue);
template <typename T = cpp_dec_float_100> T evaluateLcm(Token &arg, const T xValue);

template <typename T> void evaluateArgs(Token &arg, const T xValue, std::vector<T>&intermediateResults, size_t argsToEval=SIZE_MAX);


template <typename T> Point decimalToFraction(T enumerator);

bool addIdentifier(Variable newConstant);
bool addIdentifier(Macro newMacro);
bool replaceMacros(std::string &equation);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool mainLoop(Options &options, bool passedInAsArg,bool passedCalculationsFile, std::string &equation, std::string &resultHistory, std::string &result, std::vector<Variable> &userVariables, std::vector<Macro> &userMacros, bool canDeclareIdentifiers=false)
{

    globals::userVariables=userVariables;
    globals::userMacros=userMacros;

    globals::aroundLeniency=options;
    std::cout.precision(100);
    bool firstPass{true};
    std::ostringstream resultAsOSStream;
    resultAsOSStream.precision(100);
    globals::previousResult="nan";
    if(options.ans!="") globals::previousResult=options.ans;
    std::cout.precision(MAXOUTPUTPRECISION);
    resultAsOSStream.precision(MAXOUTPUTPRECISION);

        passedInFile:
        if(equation.find('#') != std::string::npos) equation.erase(equation.find('#'));
        if(equation.find("how do i exit vim")!=std::string::npos||equation.find("how to exit vim")!=std::string::npos)
        {
            result+=":q\n\n";
            return 1;
        }

        if(equation.length()==0) return true;
                              
        passedInAsArg:

        for(size_t i{}; i<equation.length(); i++) // Replace some unicode
        {
            if(equation.find("≤",i)==i) equation.replace(i,sizeof("≤")-1,"<=");
            else if(equation.find("ᵉ",i)==i) equation.replace(i,sizeof("ᵉ")-1,"ec");
            else if(equation.find("α",i)==i) equation.replace(i,sizeof("α")-1,"a");
            else if(equation.find("τ",i)==i) equation.replace(i,sizeof("τ")-1,"tau");
            else if(equation.find("ξ",i)==i) equation.replace(i,sizeof("ξ")-1,"rnd");
            else if(equation.find("∞",i)==i) equation.replace(i,sizeof("∞")-1,"inf");
            else if(equation.find("φ",i)==i) equation.replace(i,sizeof("φ")-1,"phi");
            else if(equation.find("√",i)==i) equation.replace(i,sizeof("√")-1,"sqrt");
            else if(equation.find("∛",i)==i) equation.replace(i,sizeof("∛")-1,"cbrt");
            else if(equation.find("∜",i)==i) equation.replace(i,sizeof("∜")-1,"qtrt");
            else if(equation.find("⊕",i)==i) equation.replace(i,sizeof("⊕")-1,"XOR");
            else if(equation.find("∨",i)==i) equation.replace(i,sizeof("∨")-1,"OR");
            else if(equation.find("∧",i)==i) equation.replace(i,sizeof("∧")-1,"AND");
            else if(equation.find("≥",i)==i) equation.replace(i,sizeof("≥")-1,">=");
            else if(equation.find("−",i)==i) equation.replace(i,sizeof("−")-1,"-");    
            else if(equation.find("≠",i)==i) equation.replace(i,sizeof("≠")-1,"=!"); 
            else if(equation.find("÷",i)==i) equation.replace(i,sizeof("÷")-1,"/"); 
            else if(equation.find("×",i)==i) equation.replace(i,sizeof("×")-1,"*"); 
            else if(equation.find("π",i)==i) equation.replace(i,sizeof("π")-1,"pi");
            else if(equation.find("γ",i)==i) equation.replace(i,sizeof("γ")-1,"eul"); 
            else if(equation.find("ℯ",i)==i) equation.replace(i,sizeof("ℯ")-1,"e");
            else if(equation.find("ᵉ",i)==i) equation.replace(i,sizeof("ᵉ")-1,"ec");
            else if(equation.find("≈",i)==i) equation.replace(i,sizeof("≈")-1,"AROUND");
            else if(equation.find("H₀",i)==i) equation.replace(i,sizeof("H₀")-1,"H0");
            else if(equation.find("Z₀",i)==i) equation.replace(i,sizeof("Z₀")-1,"Z0");
            else if(equation.find("U₀",i)==i) equation.replace(i,sizeof("U₀")-1,"U0");
            else if(equation.find("mₐ",i)==i) equation.replace(i,sizeof("mₐ")-1,"ma");
        }
        for(size_t i{}; i<equation.length(); i++) if(equation.at(i)<32) equation.erase(i--,1); // Delete unprintable characters
        
        if(equation.find("fishthumbs")!=std::string::npos) // Fish.
        {                                   
            result+="tacky could never";         
            return 0;                       
        }  

        if(equation.find("fish")!=std::string::npos) // Fish.
        {                                   
            result+="fish.";         
            return 0;                       
        }  
        
        if(equation.find("nine plus ten")!=std::string::npos)
        {                                   
            result+="twenty one.";         
            return 0;                       
        }  

        for(int i{}; i<equation.length(); i++)
        {
            if(!(isValidInput(equation.at(i)))) equation.erase(equation.begin()+i--); // Basic garbage removal
            if(i>=0)
            {
                if(equation.at(i)=='[') equation.at(i)='('; // Cheating
                else if(equation.at(i)==']') equation.at(i)=')';
                else if(equation.at(i)==';') equation.at(i)=',';
            }
        }

        if(replaceMacros(equation))
        {
            equation.clear();
            if(passedCalculationsFile) return false;
            return false;
        }

        int parenthesesImbalance{};
        uint absValueLineCount{};
        for(size_t i{}; i<equation.length(); i++)
        {
            if(equation.at(i)=='|') absValueLineCount++;
            if(equation.at(i)=='(') parenthesesImbalance++;
            else if(equation.at(i)==')') parenthesesImbalance--;
            if(parenthesesImbalance<0 || (equation.length()==i+1 && absValueLineCount%2!=0))
            {
                result+="Parentheses are not balanced!";
                equation.clear();
            }
        }

        if(absValueLineCount%2!=0||parenthesesImbalance<0) return false;
        
        if(equation.length()==0)
        {
            result+="No valid input";
            equation.clear();
            return false;
        }
        std::vector<Token> tokens = getTokens(equation);

        // Add identifiers
        for(size_t i{}; i<tokens.size() && canDeclareIdentifiers; i++)
        {
            if(tokens.at(i).typeCategory()==tokenCategory_t::ASSIGNMENT)
            {
                size_t j{};
                std::vector<Token> assignmentTokens;
                bool invalidName{};
                std::string identifierName{tokens.at(i).value().substr(3,tokens.at(i).value().length()-4)};
                if(globals::constants.find(identifierName)!=globals::constants.end() ||
                    tokens.at(i).value().find("variable")!=std::string::npos||
                    tokens.at(i).value().find("macro")!=std::string::npos) invalidName=true;
                
                if(invalidName)
                {
                    result+="Forbidden name\n";
                    tokens.clear();
                    invalidName=true;
                    break;
                }

                std::vector<Token> nameCheckTokens{getTokens(tokens.at(i).value().substr(3,tokens.at(i).value().length()-4))};
                for(size_t h{}; h<nameCheckTokens.size(); h++)
                {
                    if(nameCheckTokens.at(h).typeCategory()==tokenCategory_t::FUNCTION || nameCheckTokens.at(h).typeCategory()==tokenCategory_t::OPERATOR || nameCheckTokens.at(h).type()==token_t::NUMBER || nameCheckTokens.at(h).type()==token_t::VARIABLE)
                    {
                        result+="Forbidden name\n";
                        tokens.clear();
                        invalidName=true;
                        break;
                    }
                }
                if(invalidName) break;

                for(j=i+1; j<tokens.size() && tokens.at(j).type()!=token_t::ASSIGNMENTMACRO && tokens.at(j).type()!=token_t::VARIABLE; j++)
                {
                    if(tokens.at(i).type()==token_t::ASSIGNMENTVARIABLE && tokens.at(j).type()==token_t::ASSIGNMENTVARIABLE) break;
                    assignmentTokens.emplace_back(tokens.at(j));
                }
                if(tokens.at(i).type()==token_t::ASSIGNMENTVARIABLE) resultAsOSStream<<calculation<cpp_dec_float_100>(assignmentTokens, NAN);
                else if(tokens.at(i).type()==token_t::ASSIGNMENTMACRO)
                {
                    resultAsOSStream<<equation.substr(equation.find(tokens.at(i).value())+tokens.at(i).value().length());
                }
                bool failed{};
                
                if(resultAsOSStream.str().find("nan")==std::string::npos && 
                   tokens.at(i).type()==token_t::ASSIGNMENTVARIABLE &&
                   identifierName!=resultAsOSStream.str()) failed=addIdentifier(Variable(std::string(identifierName),resultAsOSStream.str()));
                
                else if(resultAsOSStream.str().find("nan")==std::string::npos &&
                        tokens.at(i).type()==token_t::ASSIGNMENTMACRO &&
                        identifierName!=resultAsOSStream.str()) failed=addIdentifier(Macro(std::string(identifierName),resultAsOSStream.str()));
        
                if(!failed &&
                resultAsOSStream.str().find("nan")==std::string::npos &&
                identifierName!=resultAsOSStream.str()) result+="Assigned \"" + identifierName + "\" value " + resultAsOSStream.str()+'\n';
                else result+="Cannot duplicate names or assign nan\n";

                tokens.erase(tokens.begin()+i,tokens.begin()+j-i);
                // globals::previousResult=resultAsOSStream.str();
                resultAsOSStream.str("");
                resultAsOSStream.clear();
                i--;
            }
        }

        bool hasX{};
        
        if(tokens.size()==0) 
        {
            resultAsOSStream.str("");
            resultAsOSStream.clear();
            equation.clear();
            tokens.clear();
            options.graph=false;
            firstPass=false;
            globals::tokenMemory.clear();
            getTokens("",true);

            userMacros=globals::userMacros;
            userVariables=globals::userVariables;
            return false;
        }
        // if(canDeclareIdentifiers) return false;

        for(int i{}; i<equation.length(); i++)
        {
            if(i==1 && equation.at(1)=='x' && equation.find("exp",0)!=0) hasX=true;
            if(i>1&&equation.at(i)=='x' && equation.find("max",i-2)!=i-2 && equation.find("mix",i-2)!=i-2 && equation.find("exp",i-1)!=i-1) hasX=true;
        }
        if(equation.at(0)=='x') hasX=true;

        if(!hasX && !(canDeclareIdentifiers && !passedCalculationsFile)) // No x found
        {
            resultAsOSStream<<calculation<cpp_dec_float_100>(tokens, NAN);

            if(resultAsOSStream.str().find("nan")!=std::string::npos)
            {
                globals::previousResult="nan";
                resultAsOSStream.str("");
                resultAsOSStream.clear();
                resultAsOSStream<<"Not a Number";
            }
            else if(resultAsOSStream.str()=="-0")
            {
                globals::previousResult='0';
                resultAsOSStream.str("");
                resultAsOSStream.clear();
                resultAsOSStream<<"0";               
            }
            else globals::previousResult=resultAsOSStream.str();

            for(size_t i{}; i<tokens.size(); i++)
            {
                if ((tokens.at(i).value()=="<" || 
                        tokens.at(i).value()==">" ||
                        tokens.at(i).value()=="=" || 
                        tokens.at(i).value()=="=!" ||
                        tokens.at(i).value()=="<=" || 
                        tokens.at(i).value()=="OR" ||
                        tokens.at(i).value()=="AND" ||
                        tokens.at(i).value()=="AROUND" ||
                        tokens.at(i).value()=="XOR" ||
                        tokens.at(i).value()=="NOR" ||    
                        tokens.at(i).value()==">="))
                {
                    if(resultAsOSStream.str()=="1") resultAsOSStream.str("true");
                    else if(resultAsOSStream.str()=="0") resultAsOSStream.str("false");
                }
            }

            if(!passedCalculationsFile) result=resultAsOSStream.str();
            else
            {
                result+=equation+" = "+resultAsOSStream.str()+'\n';
            }
        }
        else if(!options.graph && !(canDeclareIdentifiers && !passedCalculationsFile))
        {
            // result="";
            std::cout.precision(MAXOUTPUTPRECISION);
            resultAsOSStream.precision(MAXOUTPUTPRECISION);
            if(options.xStep==0)options.xStep=INFINITY;
            for(cpp_dec_float_100 xValue=options.xMin; xValue<=options.xMax+0.000001; xValue+=options.xStep)
            {
                std::ostringstream xValueAsOSStream;
                if(xValue>(-0.00002) && xValue<0.00002) xValue=0;
                xValueAsOSStream<<xValue;
                xValue=static_cast<cpp_dec_float_100>(xValueAsOSStream.str());

                // if(abs(xValue)-abs(round(xValue))<0.00001) xValue=round(xValue);

                resultAsOSStream<<calculation<cpp_dec_float_100>(tokens, xValue);
                if(resultAsOSStream.str().find("nan")!=std::string::npos)
                {
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    continue;
                }
                else if(resultAsOSStream.str()=="-0")
                {
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();       
                    resultAsOSStream<<"0";      
                }
                for(size_t i{}; i<tokens.size(); i++)
                {
                    if ((tokens.at(i).value()=="<" || 
                         tokens.at(i).value()==">" ||
                         tokens.at(i).value()=="=" || 
                         tokens.at(i).value()=="=!" ||
                         tokens.at(i).value()=="<=" ||
                         tokens.at(i).value()=="OR" || 
                         tokens.at(i).value()=="AND" || 
                         tokens.at(i).value()=="XOR" || 
                         tokens.at(i).value()=="AROUND" ||
                         tokens.at(i).value()=="NOR" || 
                         tokens.at(i).value()==">="))
                    {
                        if(resultAsOSStream.str()=="1") resultAsOSStream.str("true");
                        else if(resultAsOSStream.str()=="0") resultAsOSStream.str("false");
                    }
                }
                result+="For x = " + xValueAsOSStream.str() + ": " + resultAsOSStream.str()+'\n';
                resultAsOSStream.str("");
                resultAsOSStream.clear();
            }
        }
        else if(!(canDeclareIdentifiers && !passedCalculationsFile))
        {
            globals::points.first.clear();
            globals::points.second.clear();

            uint threadCount{std::thread::hardware_concurrency()};
            std::vector<std::future<std::vector<Point>>> results;
            if(globals::aroundLeniency.xStep<=0.0000000001) globals::aroundLeniency.xStep=0.0000000001;

            for(size_t i{}; i<threadCount; i++)
            {
                double perThreadRange{abs(options.xMax-options.xMin)/threadCount};
                double thisThreadOffset{perThreadRange*i};
                double thisThreadXValue{static_cast<float>(options.xMin)+thisThreadOffset};
                results.push_back(std::async(calculationCaller,std::ref(tokens),thisThreadXValue,perThreadRange+thisThreadXValue, i));
            }

            for(size_t i{}; i<threadCount; i++)
            {
                std::vector<Point> thisThreadPoints=results.at(i).get();
                for(size_t j{}; j<thisThreadPoints.size(); j++)
                {
                    globals::points.first.emplace_back(thisThreadPoints.at(j).x);
                    globals::points.second.emplace_back(thisThreadPoints.at(j).y);
                }
            }    
        }
        bool isKnownConstant{};
        if(!hasX && options.showFractions && globals::valueToConstants.find(resultAsOSStream.str())!=globals::valueToConstants.end())
        {
            if(equation!=globals::valueToConstants.find(resultAsOSStream.str())->second)
            {
                if(!passedCalculationsFile) result=globals::valueToConstants.find(resultAsOSStream.str())->second;
                else
                {
                    result+=equation+" = "+globals::valueToConstants.find(resultAsOSStream.str())->second+'\n';
                }
                isKnownConstant=true;
            }

        }

        if(!hasX && options.showFractions && isNumber(resultAsOSStream.str()) && !isKnownConstant && resultAsOSStream.str().find('e')==std::string::npos)
        {
            Point frac = decimalToFraction(std::stod(resultAsOSStream.str()));
            resultAsOSStream.str("");
            resultAsOSStream<<frac.x<<'/'<<frac.y;

            if(abs(frac.x)!=INFINITY && frac.y!=1 && resultAsOSStream.str().length()<12 && equation!=resultAsOSStream.str())
            {
                if(!passedCalculationsFile) result=resultAsOSStream.str();
                else result+=equation+" = "+resultAsOSStream.str()+'\n';
            }
        }


        if(!hasX && !(canDeclareIdentifiers && !passedCalculationsFile))
        {
            std::string addToHistory = '\n'+equation+" = "+result;
            if(resultHistory.find(addToHistory)==std::string::npos) resultHistory+=addToHistory;
        }
        cleanup:
        resultAsOSStream.str("");
        resultAsOSStream.clear();
        equation.clear();
        tokens.clear();
        options.graph=false;
        firstPass=false;
        // globals::tokenMemory.clear();
        getTokens("",true);

        userMacros=globals::userMacros;
        userVariables=globals::userVariables;


        return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline std::vector<Point> calculationCaller(std::vector<Token> &tokens, double xValue, double xValueMax, size_t threadNumber)
{
    if(threadNumber==std::thread::hardware_concurrency()-1) xValueMax+=static_cast<float>(globals::aroundLeniency.xStep)*5;
    std::vector<Point> points;
    for(;xValue<xValueMax; xValue+=static_cast<float>(globals::aroundLeniency.xStep))
    {
        points.emplace_back(xValue,calculation<double>(tokens,xValue));
    }
    return points;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isValidInput(const char c)
{
    return !(c=='\t' || c=='\n' || c==' ');

            /*(c>='0'&&c<='9')||c=='.'||c=='x'||c=='+'||c=='-'||c=='*'||c=='/'||c=='('||c==')'||c=='^'||c=='!'||c=='r'||c=='o'||c=='t'
            ||c==','||c=='e'||c=='s'||c=='i'||c=='n'||c=='c'||c=='a' ||c=='l'||c=='f'||c=='u'||c=='d'||c=='|'||c=='b'||c=='g'||c=='p'
            ||c=='u'||c=='h'||c=='m'||c=='%'||c=='k'||c=='['||c==']'||c=='h'||c=='G'||c=='H'||c==';'||c=='Z'||c=='U'||c=='E'||c=='R'
            ||c=='N'||c=='v'||c=='='*/
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void parseMultiArgFunction(const std::string &input, std::vector<Token> &tokens, const char* functionName, size_t &i, bool &inFunctionCall, size_t argCount)
{
    size_t initialI{i};
    i=0;
    size_t argFound{1};
    std::string currentToken;
    int nestingLevel{};
    bool done{};
    size_t functionNameLength{};
    for(; functionName[functionNameLength]!='\000'; functionNameLength++);

    for(; i<input.length(); i++)
    {
        if(currentToken=="" && input.find(functionName, i)==i) for(; i<input.length(); i++)
        {
            if(!inFunctionCall)
            {
                currentToken.append(functionName);
                i+=functionNameLength;
                inFunctionCall=true;
                if(i==input.length()-1) continue;
            }
            if(input.at(i)==',' && nestingLevel==1) argFound++;
            if(argFound>argCount)
            {
                for(; i<input.length() && nestingLevel>0; i++)
                {
                    if(input.at(i)==')') nestingLevel--;
                    else if(input.at(i)=='(') nestingLevel++;                    
                }
                break;
            }
            if(input.at(i)==')') nestingLevel--;
            else if(input.at(i)=='(') nestingLevel++;
            currentToken.push_back(input.at(i));
            if((i==input.length()-2 && input.at(i)==',' && input.at(i+1)==')') || (input.at(i-1)==',' && input.at(i)==',') || (nestingLevel==0 && input.at(i-1)==',')) //Check for some bad argument cases
            {
                currentToken.clear();
                continue;
            }
            if(nestingLevel==0 || i==input.length()-1)
            {
                tokens.emplace_back(currentToken);
                done=true;
                i+=initialI;
                return;
            }
        }
    }
    i+=initialI;
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline std::vector<Token> getTokens(const std::string &input, bool resetFirstRun)
{
    std::ostringstream resultAsOSStream;
    resultAsOSStream.precision(MAXOUTPUTPRECISION);

    if(globals::tokenMemory.find(input)!=globals::tokenMemory.end())
    {
        return globals::tokenMemory.find(input)->second;
    }
    static bool firstRun{true};
    if(resetFirstRun)
    {
        firstRun=true;
        return std::vector<Token>();
    }
    static std::string_view lastSeenResult{};
    if(globals::previousResult!="nan") lastSeenResult=globals::previousResult;
    int nestingLevel{};
    int absNestingLevel{};
    int nestingOfFunction{};
    uint startOfFunction{};
    uint endOfFirstArg{};
    std::vector<Token> tokens{};
    
    std::string currentToken{};
    bool fixOffByOne{};
    bool inFunctionCall{};

    for(size_t i{}; i<input.length(); i++)
    {
        // Parse |x|... or ||x|| if the user hates me... or ||||x||||. whatever.
        if(currentToken=="" && input.at(i)=='|') for(startOfFunction=i; i<input.length(); i++)
        {
            if(!inFunctionCall)
            {
                startOfFunction=i;
                for(;input.at(i)=='|' && i<input.length()-1;i++)
                {
                    absNestingLevel++;
                    currentToken.push_back('|');
                }
                inFunctionCall=true;
                nestingOfFunction=nestingLevel;
                if(i>=input.length()-1) break;
            }
            if(input.at(i)==')')
            {
                nestingLevel--;
                nestingLevel--;
            }
            else if(input.at(i)=='(')
            {
                nestingLevel++;
                nestingLevel++;
            }
            if(i<input.length() && nestingLevel==false && input.at(i)=='|') absNestingLevel--;
            if(i>startOfFunction+1 && nestingLevel<=0 && absNestingLevel==0 && nestingLevel==false && input.at(i)=='|' || 
               (i==input.length()-1 && input.at(i)=='|')) 
            {
                currentToken.push_back(input.at(i));
                tokens.emplace_back(currentToken);
                break;
            }
            else if(i==input.length()-1 && input.at(i)!='|') continue;

            currentToken.push_back(input.at(i));
        }

        // Parse MultiArg Functions
        if(!inFunctionCall && input.find("mean(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"mean",i,inFunctionCall);
        else if(!inFunctionCall && input.find("derive(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"derive",i,inFunctionCall);
        else if(!inFunctionCall && input.find("median(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"median",i,inFunctionCall);
        else if(!inFunctionCall && input.find("stdevp(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"stdevp",i,inFunctionCall);
        else if(!inFunctionCall && input.find("max(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"max",i,inFunctionCall);
        else if(!inFunctionCall && input.find("sabs(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"sabs",i,inFunctionCall);
        else if(!inFunctionCall && input.find("smin(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"smin",i,inFunctionCall);
        else if(!inFunctionCall && input.find("smax(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"smax",i,inFunctionCall);
        else if(!inFunctionCall && input.find("gcf(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"gcf",i,inFunctionCall);
        else if(!inFunctionCall && input.find("hcf(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"hcf",i,inFunctionCall);
        else if(!inFunctionCall && input.find("hcd(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"hcd",i,inFunctionCall);
        else if(!inFunctionCall && input.find("gcd(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"gcd",i,inFunctionCall);
        else if(!inFunctionCall && input.find("mix(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"mix",i,inFunctionCall);
        else if(!inFunctionCall && input.find("lcm(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"lcm",i,inFunctionCall);
        else if(!inFunctionCall && input.find("min(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"min",i,inFunctionCall);
        else if(!inFunctionCall && input.find("rndsel(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"rndsel",i,inFunctionCall);
        else if(!inFunctionCall && input.find("rndint(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"rndint",i,inFunctionCall);
        else if(!inFunctionCall && input.find("root(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"root",i,inFunctionCall);
        else if(!inFunctionCall && input.find("log(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"log",i,inFunctionCall);
        else if(!inFunctionCall && input.find("if(", i)==i) parseMultiArgFunction(input.substr(i),tokens,"if",i,inFunctionCall);

        // Parse Subexpression
        if(currentToken=="" && !inFunctionCall && input.at(i)=='(') for(; i<input.length(); i++)
        {
            if(input.at(i)==')') nestingLevel--;
            else if(input.at(i)=='(') nestingLevel++;
            if(nestingLevel!=0)currentToken.push_back(input.at(i));
            if(nestingLevel==0 || i==input.length()-1) break;                
        }

        // Parse assignment
        if(currentToken=="" && !inFunctionCall && firstRun && (input.find("let",i)==i || input.find("set",i)==i)&& input.find('=',i)!=std::string::npos)
        {
            if(input.find("let",i)==i) currentToken.append("let");
            else currentToken.append("set");

            for(i+=3; i<input.length() && input.at(i)!='x' && input.find("set",i)!=i && input.at(i)!='='; i++) currentToken.push_back(input.at(i));
            if(currentToken=="let" || currentToken=="set")
            {
                currentToken.clear();
                goto cleanup;
            }
            currentToken.push_back('=');
            goto cleanup;
        }
        
        // Parse other symbols
        if(currentToken=="" && !inFunctionCall)
        {
            size_t k{};
            for(; k<globals::userMacros.size(); k++)
            {
                if(input.find(globals::userMacros.at(k).name,i)==i)
                {
                    currentToken=globals::userMacros.at(k).name;
                    i+=globals::userMacros.at(k).name.length()-1;
                }
            }
            if(k && currentToken!="") goto cleanup;

            size_t j{};
            for(; j<globals::userVariables.size(); j++)
            {
                if(input.find(globals::userVariables.at(j).name,i)==i)
                {
                    currentToken=globals::userVariables.at(j).name;
                    i+=globals::userVariables.at(j).name.length()-1;
                }
            } 
            if(j && currentToken!="") goto cleanup;
            
            std::string candidate;
            for(size_t j{MAXKEYWORDLENGTH}; j>0; j--) // Check short substrs in front of where you are in equation in descending size and match against known symbols
            {
                candidate = input.substr(i,j);
                if(globals::symbols.find(candidate)!=globals::symbols.end())
                {
                    currentToken=candidate;
                    i+=j-1;
                    break;
                }
            }


        }

        // Parse Number
        if(currentToken=="")for(; i<input.length() &&
                                  (std::isdigit(input.at(i)) || // 5
                                  
                                  (i<input.length()-1 && input.at(i)=='.' && std::isdigit(input.at(i+1))) || // .5

                                  (i>0 && std::isdigit(input.at(i-1)) &&
                                   input.at(i)=='e' && 
                                   i<input.length()-2 &&
                                   (input.at(i+1)=='+' || input.at(i+1)=='-') &&
                                   std::isdigit(input.at(i+2)))); i++) // 5e±5

                                   // I am deeply sorry.
        {
            fixOffByOne=true;
            if(i+1<input.length() &&
              (input.at(i)=='e' &&
              (input.at(i+1)=='+' || input.at(i+1)=='-')))
            {
                currentToken.push_back(input.at(i));
                i++;
            }
            currentToken.push_back(input.at(i));
        }
        if(fixOffByOne)
        {
            fixOffByOne=false;
            i--;
        }

        cleanup:
        if(inFunctionCall) currentToken.clear();
        if(currentToken!="") tokens.emplace_back(currentToken);
        currentToken.clear();
        inFunctionCall=false;
        startOfFunction=0;
    }
    if(currentToken!="") tokens.emplace_back(currentToken);
    if(firstRun) firstRun=false;

    
    
    // Remove some stray operators
    if(tokens.at(0).typeCategory()==tokenCategory_t::OPERATOR && tokens.at(0).value()!="-") tokens.erase(tokens.begin());

    // Implicit multiplication, removing unary plus, so on

    for(size_t i{1}; i<tokens.size(); i++)
    {
        // Case example: 4!!3 -> 4!! h* 3
        if(tokens.at(i).typeCategory()==tokenCategory_t::NUMBER && 
        (tokens.at(i-1).type()==token_t::UNARYOP && tokens.at(i-1).value()!="-" || tokens.at(i-1).value()=="!!"))
                tokens.emplace(tokens.begin()+i++, Token("h*"));

        // Case example: 3x -> 3 h* x
        if((tokens.at(i).type()==token_t::VARIABLE || tokens.at(i).type()==token_t::CONSTANT) &&
            tokens.at(i-1).typeCategory()==tokenCategory_t::NUMBER) tokens.emplace(tokens.begin()+i++, Token("h*"));

        // Case example: x3 -> x h* 3
        if(tokens.at(i).typeCategory()==tokenCategory_t::NUMBER &&
        (tokens.at(i-1).type()==token_t::VARIABLE || tokens.at(i-1).type()==token_t::CONSTANT)) tokens.emplace(tokens.begin()+i++, Token("h*"));

        // Case example: 3sin x -> 3 h* sinx
        if(tokens.at(i).typeCategory()==tokenCategory_t::FUNCTION &&
        tokens.at(i-1).typeCategory()==tokenCategory_t::NUMBER) tokens.emplace(tokens.begin()+i++, Token("h*"));
        
        // Case example: 3(expr) -> 3 h* (expr)
        if((tokens.at(i).typeCategory()==tokenCategory_t::SUBEXPR || tokens.at(i).typeCategory()==tokenCategory_t::FUNCTION) &&
            tokens.at(i-1).typeCategory()!=tokenCategory_t::OPERATOR &&
            tokens.at(i-1).typeCategory()!=tokenCategory_t::FUNCTION) tokens.emplace(tokens.begin()+i++, Token("h*"));

        // Case example: (expr)3 -> (expr) h* 3 
        if(tokens.at(i-1).typeCategory()==tokenCategory_t::SUBEXPR && 
        tokens.at(i).typeCategory()!=tokenCategory_t::OPERATOR &&
        tokens.at(i).typeCategory()!=tokenCategory_t::SUBEXPR) tokens.emplace(tokens.begin()+i++, Token("h*"));

        // Case example: 3-3 -> 3+-3, (expr)-3 -> (expr)+-3
        // Reason: Binary minus is a lie lol
        if(tokens.at(i).value()=="-" &&
        tokens.at(i-1).type()!=token_t::BINARYOP &&
        tokens.at(i-1).type()!=token_t::UNARYOP &&
        tokens.at(i-1).type()!=token_t::FUNCTION) tokens.emplace(tokens.begin()+i++, Token("+"));

        // Delete unary plus since it does jack
        if(tokens.at(i).value()=="+" &&
        tokens.at(i-1).typeCategory()!=tokenCategory_t::NUMBER &&
        tokens.at(i-1).typeCategory()!=tokenCategory_t::SUBEXPR) tokens.erase(tokens.begin()+i--);
    }
    
    for(int i{1}; i<tokens.size(); i++)
    {
        if(tokens.at(i).type()==token_t::BINARYOP && tokens.at(i-1).type()==token_t::BINARYOP) // Example: 3**/3 -> 3**3
        {
            tokens.erase(tokens.begin()+i--);
        }
        if(i>0 && tokens.at(i).value()=="-" && tokens.at(i-1).value()=="-")
        {
            tokens.erase(tokens.begin()+i-1,tokens.begin()+i+1);
            i-=2;
        }
    }
    for(int i{static_cast<int>(tokens.size())-1}; i>=0 ; i--)
    {
        if(tokens.at(i).value()=="-" ||
        tokens.at(i).type()==token_t::BINARYOP ||
        tokens.at(i).typeCategory()==tokenCategory_t::FUNCTION) tokens.erase(tokens.begin()+i);

        else break;
    }

    // Replace ans, rnd, rndint with numbers
    for(size_t i{}; i<tokens.size(); i++)
    {
        if(tokens.at(i).value()=="ans")
        {
            tokens.at(i)=Token(globals::previousResult);
        }
        if(tokens.at(i).value()=="rnd" || tokens.at(i).value()=="rndint")
        {
            std::uniform_real_distribution<> doubleDist(0,1);
            resultAsOSStream<<doubleDist(randomMt);
            std::string randomAsStr {resultAsOSStream.str()};
            if(tokens.at(i).value()=="rndint") // To get random integers, it literally deletes the decimal point
            {
                randomAsStr.erase(randomAsStr.find('.'), 1);
            }
            tokens.at(i)=Token(randomAsStr);
            resultAsOSStream.str("");
            resultAsOSStream.clear();
        }
    }


    globals::tokenMemory.emplace(input,tokens);
    return tokens;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T calculation(std::vector<Token> tokens, const T xValue)
{
    if(tokens.size()==0) return 0;
    std::ostringstream resultAsOSStream;
    if(std::is_same_v<T,cpp_dec_float_100>) resultAsOSStream.precision(MAXOUTPUTPRECISION);
    else resultAsOSStream.precision(15);
        
    


    if(tokens.size()==1 && tokens.at(0).typeCategory()==tokenCategory_t::NUMBER) return tokens.at(0).number(xValue);
    if(tokens.size()==1 && tokens.at(0).type()==token_t::INVALID) return NAN;
    size_t pass{};
    size_t failedPass{LOGICALS};
    for(; pass<=LOGICALS; pass++)
    {
        for(int i{}; i<tokens.size(); i++)
        {
            if(pass==SUBEXPRESSIONS)
            {
                T evaluatedSubexpr{NAN};
                if(tokens.at(i).type()==token_t::SUBEXPR) evaluatedSubexpr=calculation(getTokens(tokens.at(i).value()), xValue);
                else if(tokens.at(i).type()==token_t::DERIVE) evaluatedSubexpr=evaluateDerive(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::MEAN) evaluatedSubexpr=evaluateMean(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::MEDIAN) evaluatedSubexpr=evaluateMedian(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::STDEVP) evaluatedSubexpr=evaluateStdevp(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::MAX) evaluatedSubexpr=evaluateMax(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::MIX) evaluatedSubexpr=evaluateMix(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::SABS) evaluatedSubexpr=evaluateSabs(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::SMAX) evaluatedSubexpr=evaluateSmax(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::SMIN) evaluatedSubexpr=evaluateSmin(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::GCF) evaluatedSubexpr=evaluateGcf(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::LCM) evaluatedSubexpr=evaluateLcm(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::MIN) evaluatedSubexpr=evaluateMin(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::RNDSEL) evaluatedSubexpr=evaluateRndsel(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::RNDINT) evaluatedSubexpr=evaluateRndint(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::ABS) evaluatedSubexpr=evaluateAbs(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::ROOT) evaluatedSubexpr=evaluateRoot(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::LOG) evaluatedSubexpr=evaluateLog(tokens.at(i), xValue);
                else if(tokens.at(i).type()==token_t::IF) evaluatedSubexpr=evaluateIf(tokens.at(i), xValue);


                if(tokens.at(i).typeCategory()==tokenCategory_t::SUBEXPR)
                {
                    resultAsOSStream<<evaluatedSubexpr;
                    tokens.at(i)=Token(resultAsOSStream.str());
                    resultAsOSStream.str("");
                    resultAsOSStream.clear(); 
                }
            }
            else if(pass==UNARYOPS)
            {
                if(i==0)
                {
                    for(uint j{}; j<tokens.size(); j++)
                    {
                        if(tokens.at(j).type()==token_t::SUBEXPR) failedPass=SUBEXPRESSIONS;
                    }
                    continue;
                }
                if((tokens.at(i).type()==token_t::UNARYOP || tokens.at(i).type()==token_t::UNARYOP) && tokens.at(i-1).typeCategory()==tokenCategory_t::NUMBER)
                {
                    T evaluatedUnary=evaluateUnary(tokens.at(i-1), tokens.at(i), xValue);
                    resultAsOSStream << evaluatedUnary;
                    tokens.at(i-1)=Token(resultAsOSStream.str());
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    tokens.erase(tokens.begin()+i);
                    i--;
                }
            }
            else if(pass==EXPONENTIATION)
            {
                if(i==0)
                {
                    for(uint j{}; j<tokens.size(); j++)
                    {
                        if(tokens.at(j).type()==token_t::UNARYOP && failedPass==ADDITION) failedPass=UNARYOPS;
                    }
                    
                    for(i=tokens.size()-1; i>0; i--)
                    {
                        if(i-2<tokens.size())
                        {
                            // Account for something like x^-1
                            if((tokens.at(i-2).value()=="^" || tokens.at(i-1).value()=="**") && tokens.at(i-1).value()=="-" && tokens.at(i).typeCategory()==tokenCategory_t::NUMBER)
                            {
                                T evaluatedUnary=evaluateUnary(tokens.at(i), tokens.at(i-1), xValue);
                                resultAsOSStream << evaluatedUnary;
                                tokens.at(i-1)=Token(resultAsOSStream.str());
                                resultAsOSStream.str("");
                                resultAsOSStream.clear();
                                tokens.erase(tokens.begin()+i);                               
                            }
                            if(tokens.at(i-2).typeCategory()==tokenCategory_t::NUMBER && (tokens.at(i-1).value()=="^" || tokens.at(i-1).value()=="**") && tokens.at(i).typeCategory()==tokenCategory_t::NUMBER)
                            {
                                T evaluatedBinary=evaluateBinary(tokens.at(i-2), tokens.at(i-1), tokens.at(i), xValue);
                                resultAsOSStream << evaluatedBinary;
                                tokens.at(i-2)=Token(resultAsOSStream.str());
                                tokens.erase(tokens.begin()+i-1);
                                tokens.erase(tokens.begin()+i-1);
                                resultAsOSStream.str("");
                                resultAsOSStream.clear();
                            }
                        }
                    }
                }
            }
            else if (pass==FUNCTIONS)
            {
                if(i==0)
                    for(uint j{}; j<tokens.size(); j++)
                    {
                        if(tokens.at(j).value()=="^" && failedPass==ADDITION) failedPass=EXPONENTIATION;
                    }

                // Account for something like sin-1... or sqrt-1 if the user feels funny.
                if(i!=0 && i<tokens.size()-1 && tokens.at(i-1).type()==token_t::FUNCTION  && tokens.at(i).value()=="-" && tokens.at(i+1).typeCategory()==tokenCategory_t::NUMBER)
                {
                    tokens.at(i)=Token('-'+tokens.at(i+1).value());
                    resultAsOSStream << evaluateUnary(tokens.at(i), tokens.at(i-1), xValue);
                    tokens.at(i-1)=Token(resultAsOSStream.str());
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    tokens.erase(tokens.begin()+i,tokens.begin()+i+2);                          
                }
                if(i!=0&&(tokens.at(i-1).type()==token_t::FUNCTION) && tokens.at(i).typeCategory()==tokenCategory_t::NUMBER)
                {
                    T evaluatedUnary=evaluateUnary(tokens.at(i), tokens.at(i-1), xValue);

                    if((tokens.at(i-1).value()=="sin" || tokens.at(i-1).value()=="cos") && abs(evaluatedUnary)<std::numeric_limits<T>::epsilon()) 
                    {
                        evaluatedUnary=0;
                    }

                    resultAsOSStream << evaluatedUnary;
                    tokens.at(i-1)=Token(resultAsOSStream.str());
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    tokens.erase(tokens.begin()+i);
                    i--;
                }
            }
            else if (pass==UNARYMINUS)
            {
                if(i==0)
                    for(uint j{}; j<tokens.size(); j++)
                    {
                        if(tokens.at(j).type()==token_t::FUNCTION && failedPass==ADDITION) failedPass=FUNCTIONS;
                    }
                if(i!=0&&(tokens.at(i-1).value()=="-") && tokens.at(i).typeCategory()==tokenCategory_t::NUMBER)
                {
                    T evaluatedUnary=evaluateUnary(tokens.at(i), tokens.at(i-1), xValue);
                    resultAsOSStream << evaluatedUnary;
                    tokens.at(i-1)=Token(resultAsOSStream.str());
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    tokens.erase(tokens.begin()+i);
                    i--;
                }
            }

            else if(pass==MULTIPLICATIONIMPLICIT && globals::aroundLeniency.followImplicitMultiplicationPriorityConvention)
            {
                if(i<=1) continue;
                if(tokens.at(i-2).typeCategory()==tokenCategory_t::NUMBER && (tokens.at(i-1).value()=="h*") && tokens.at(i).typeCategory()==tokenCategory_t::NUMBER)
                {
                    T evaluatedBinary=evaluateBinary(tokens.at(i-2), tokens.at(i-1), tokens.at(i), xValue);
                    resultAsOSStream << evaluatedBinary;
                    tokens.at(i-2)=Token(resultAsOSStream.str());
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    tokens.erase(tokens.begin()+i-1);
                    tokens.erase(tokens.begin()+i-1);
                    i-=2;
                }                
            }

            else if(pass==MULTIPLICATION)
            {
                if(i==0)
                    for(uint j{}; j<tokens.size(); j++)
                    {
                        if(tokens.at(j).value()=="-" && failedPass==ADDITION) failedPass=UNARYMINUS;
                    }
                if(i<=1) continue;
                if(tokens.at(i-2).typeCategory()==tokenCategory_t::NUMBER && (tokens.at(i-1).value()=="h*" || tokens.at(i-1).value()=="*" || tokens.at(i-1).value()=="/" || tokens.at(i-1).value()=="nCk" || tokens.at(i-1).value()=="nPk" || tokens.at(i-1).value()=="mod" || tokens.at(i-1).value()=="%") && tokens.at(i).typeCategory()==tokenCategory_t::NUMBER)
                {
                    T evaluatedBinary=evaluateBinary(tokens.at(i-2), tokens.at(i-1), tokens.at(i), xValue);
                    resultAsOSStream << evaluatedBinary;
                    tokens.at(i-2)=Token(resultAsOSStream.str());
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    tokens.erase(tokens.begin()+i-1);
                    tokens.erase(tokens.begin()+i-1);
                    i-=2;
                }
            }
            else if(pass==ADDITION)
            {
                if(i==0)
                    for(uint j{}; j<tokens.size(); j++)
                    {
                        if((tokens.at(j).value()=="*"||tokens.at(j).value()=="%"||tokens.at(j).value()=="mod"||tokens.at(j).value()=="nPk"||tokens.at(j).value()=="nCk"||tokens.at(j).value()=="/") && failedPass==ADDITION) failedPass=MULTIPLICATION;
                    }
                if(i<=1) continue;
                if(tokens.at(i-2).typeCategory()==tokenCategory_t::NUMBER && (tokens.at(i-1).value()=="+" || tokens.at(i-1).value()=="-") && tokens.at(i).typeCategory()==tokenCategory_t::NUMBER)
                {
                    T evaluatedBinary=evaluateBinary(tokens.at(i-2), tokens.at(i-1), tokens.at(i), xValue);
                    resultAsOSStream << evaluatedBinary;
                    tokens.at(i-2)=Token(resultAsOSStream.str());
                    tokens.erase(tokens.begin()+i-1);
                    tokens.erase(tokens.begin()+i-1);
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    i-=2;
                }
            }
            else if(pass==COMPARISONS)
            {
                if(i==0)
                    for(uint j{}; j<tokens.size(); j++)
                    {
                        if((tokens.at(j).value()=="+")) failedPass=ADDITION;
                    }
                if(i<=1) continue;
                if(tokens.at(i-2).typeCategory()==tokenCategory_t::NUMBER && (tokens.at(i-1).value()=="<" ||
                                                                                tokens.at(i-1).value()==">" || 
                                                                                tokens.at(i-1).value()==">=" || 
                                                                                tokens.at(i-1).value()=="<=" || 
                                                                                tokens.at(i-1).value()=="=" ||
                                                                                tokens.at(i-1).value()=="=!") &&
                                                                                tokens.at(i).typeCategory()==tokenCategory_t::NUMBER)
                {
                    T evaluatedBinary=evaluateBinary(tokens.at(i-2), tokens.at(i-1), tokens.at(i), xValue);
                    resultAsOSStream << evaluatedBinary;
                    tokens.at(i-2)=Token(resultAsOSStream.str());
                    tokens.erase(tokens.begin()+i-1);
                    tokens.erase(tokens.begin()+i-1);
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    i-=2;
                }
            }
            else if(pass==LOGICALS)
            {
                if(i<=1) continue;
                if(tokens.at(i-2).typeCategory()==tokenCategory_t::NUMBER && (tokens.at(i-1).value()=="AND" ||
                                                                                tokens.at(i-1).value()=="OR" || 
                                                                                tokens.at(i-1).value()=="XOR" || 
                                                                                tokens.at(i-1).value()=="AROUND" || 
                                                                                tokens.at(i-1).value()=="NOR") &&
                                                                                tokens.at(i).typeCategory()==tokenCategory_t::NUMBER)
                {
                    T evaluatedBinary=evaluateBinary(tokens.at(i-2), tokens.at(i-1), tokens.at(i), xValue);
                    resultAsOSStream << evaluatedBinary;
                    tokens.at(i-2)=Token(resultAsOSStream.str());
                    tokens.erase(tokens.begin()+i-1);
                    tokens.erase(tokens.begin()+i-1);
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                    i-=2;
                }
            }
        }
    }
    if(tokens.size()==1 && tokens.at(0).type()==token_t::VARIABLE) return xValue;
    if constexpr (std::is_same<T,cpp_dec_float_100>::value)
    {
        if(tokens.size()==1 && (tokens.at(0).type()==token_t::NUMBER|| tokens.at(0).type()==token_t::CONSTANT)) return static_cast<cpp_dec_float_100>(tokens.at(0).value());
    }
    if(tokens.size()==1 && (tokens.at(0).type()==token_t::NUMBER|| tokens.at(0).type()==token_t::CONSTANT)) return std::stold(tokens.at(0).value());
    return NAN;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateAbs(Token &arg, const T xValue)
{
    return abs(calculation<T>(getTokens(arg.value()), xValue));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateMean(Token &arg, const T xValue)
{
    T result{};
    std::vector<T> intermediateResults;
    evaluateArgs(arg,xValue,intermediateResults);   
    for(size_t i{}; i<intermediateResults.size(); i++)
    {
        result+=intermediateResults.at(i);
    }
    result=result/(intermediateResults.size());

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateMedian(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    evaluateArgs(arg,xValue,intermediateResults);

    std::sort(intermediateResults.begin(), intermediateResults.end());

    if(intermediateResults.size()%2!=0) return intermediateResults.at(intermediateResults.size()/2);
    else return (intermediateResults.at(intermediateResults.size()/2-1)+intermediateResults.at(intermediateResults.size()/2))/2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateStdevp(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    evaluateArgs(arg,xValue,intermediateResults);

    std::sort(intermediateResults.begin()+1, intermediateResults.end());

    if(intermediateResults.size()<2)
    {
        return 0;
    }
    T summedIntermediates{};
    for(size_t i{}; i<intermediateResults.size(); i++)
    {
        summedIntermediates+=intermediateResults.at(i);
    }
    const T mean{summedIntermediates/intermediateResults.size()};
    T summed{};
    for(size_t i{}; i<intermediateResults.size(); i++)
    {
        summed+=pow(intermediateResults.at(i)-mean,2);
    }


    return sqrt(summed/intermediateResults.size()); // Intellegre
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateLcm(Token &arg, const T xValue)
{
    std::ostringstream numberAsOSStream;
    std::vector<T> intermediateResults;
    T tempValue{};
    T numLeft{};
    T numRight{};
    std::string numberAsString;
    evaluateArgs(arg,xValue,intermediateResults);
    if(globals::aroundLeniency.graph)
    {
        for(size_t i{}; i<intermediateResults.size(); i++) intermediateResults.at(i)=round(intermediateResults.at(i));
    }
    for(size_t i{}; i<intermediateResults.size(); i++)
    {
        if(intermediateResults.at(i)<0) intermediateResults.at(i)=-intermediateResults.at(i);
    }
    numLeft=intermediateResults.at(0); //a
    numRight=intermediateResults.at(1); //b
    while(intermediateResults.at(1)!=0)
    {
        tempValue=intermediateResults.at(1); //b
        intermediateResults.at(1)=boost::math::ccmath::fmod(intermediateResults.at(0),intermediateResults.at(1));
        intermediateResults.at(0)=tempValue; 
    }
    intermediateResults.at(0)=(numLeft*numRight)/intermediateResults.at(0);
    intermediateResults.erase(intermediateResults.begin()+1);
    if(intermediateResults.size()>=2)
    {
        numberAsOSStream<<"lcm(";
        for(size_t i{}; i<intermediateResults.size(); i++)
        {
            numberAsOSStream<<intermediateResults.at(i);
            if(i!=intermediateResults.size()-1) numberAsOSStream<<',';
        }
        Token newArg{numberAsOSStream.str()};
        intermediateResults.at(0) = evaluateLcm(newArg,xValue);
    }
    return intermediateResults.at(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateGcf(Token &arg, const T xValue)
{
    std::ostringstream numberAsOSStream;
    std::vector<T> intermediateResults;
    T tempValue{};
    std::string numberAsString;
    evaluateArgs(arg,xValue,intermediateResults);
    if(globals::aroundLeniency.graph)
    {
        for(size_t i{}; i<intermediateResults.size(); i++) intermediateResults.at(i)=round(intermediateResults.at(i));
    }
    for(size_t i{}; i<intermediateResults.size(); i++)
    {
        if(intermediateResults.at(i)<0) intermediateResults.at(i)=-intermediateResults.at(i);
    }
    while(intermediateResults.size()>=2)
    {
        while(intermediateResults.at(1)!=0)
        {
            tempValue=intermediateResults.at(1);
            intermediateResults.at(1)=boost::math::ccmath::fmod(intermediateResults.at(0),intermediateResults.at(1));
            intermediateResults.at(0)=tempValue; 
        }
        intermediateResults.erase(intermediateResults.begin()+1);
    }
    return intermediateResults.at(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void evaluateArgs(Token &arg, const T xValue, std::vector<T>&intermediateResults, size_t argsToEval)
{
    std::string currentToken;
    int nestingLevel{};
    for(size_t i{}; i<arg.value().length() && nestingLevel>=0 && intermediateResults.size()<argsToEval; i++)
    {
        if(arg.value().at(i)=='(') nestingLevel++;
        else if(arg.value().at(i)==')') nestingLevel--;
        if(nestingLevel<0) break;
        if(!(arg.value().at(i)==',' && nestingLevel==0) && i<arg.value().length()) currentToken.push_back(arg.value().at(i));
        else
        {
            intermediateResults.emplace_back(calculation<T>(getTokens(currentToken), xValue));
            currentToken.clear();
        }
    }
    if(currentToken!="") intermediateResults.emplace_back(calculation<T>(getTokens(currentToken), xValue));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T = cpp_dec_float_100>
T evaluateRndint(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    int nestingLevel{};
    evaluateArgs(arg, xValue, intermediateResults,2);
    if(intermediateResults.size()==0) return 0;
    if(intermediateResults.size()==1) return intermediateResults.at(0);
    // std::cout<<"\nResults: "<<intermediateResults.at(0) << ',' << intermediateResults.at(1);

    if(intermediateResults.at(0)!=intermediateResults.at(0) || intermediateResults.at(1)!=intermediateResults.at(1)) return 0;

    if(round(intermediateResults.at(0)) > round(intermediateResults.at(1))) std::swap(intermediateResults.at(0), intermediateResults.at(1));

    std::uniform_int_distribution<> intDist(static_cast<int>(round(intermediateResults.at(0))),static_cast<int>(round(intermediateResults.at(1))));
    return intDist(randomMt);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateRndsel(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    evaluateArgs(arg,xValue,intermediateResults);
    std::uniform_int_distribution<size_t> intDist(0, intermediateResults.size()-1);
    return intermediateResults.at(intDist(randomMt));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateMax(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    evaluateArgs(arg,xValue,intermediateResults);
    for(size_t i{}; i<intermediateResults.size(); i++) if(intermediateResults.at(i)>intermediateResults.at(0)) intermediateResults.at(0)=intermediateResults.at(i);
    return intermediateResults.at(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateDerive(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    int nestingLevel{};
    T deriveStepSize = static_cast<T>(globals::aroundLeniency.xStep);
    if(deriveStepSize<0.05) deriveStepSize=0.05;
    for(size_t i{}; i<arg.value().length() && nestingLevel>=0 && intermediateResults.size()<2; i++)
    {
        if(arg.value().at(i)=='(') nestingLevel++;
        else if(arg.value().at(i)==')') nestingLevel--;
        if(nestingLevel<0) break;
        if(!(arg.value().at(i)==',' && nestingLevel==0) && i<arg.value().length()) currentToken.push_back(arg.value().at(i));
        else
        {
            intermediateResults.emplace_back(calculation<T>(getTokens(currentToken), xValue+deriveStepSize));
            intermediateResults.emplace_back(calculation<T>(getTokens(currentToken), xValue-deriveStepSize));
            currentToken.clear();
            break;
        }
    }
    if(currentToken!="")
    {
        intermediateResults.emplace_back(calculation<T>(getTokens(currentToken), xValue+deriveStepSize));
        intermediateResults.emplace_back(calculation<T>(getTokens(currentToken), xValue-deriveStepSize));
        currentToken.clear();  
    }
    return (intermediateResults.at(0)-intermediateResults.at(1))/(deriveStepSize*2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateMin(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    evaluateArgs(arg,xValue,intermediateResults);
    for(size_t i{}; i<intermediateResults.size(); i++) if(intermediateResults.at(i)<intermediateResults.at(0)) intermediateResults.at(0)=intermediateResults.at(i);
    return intermediateResults.at(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateSmax(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    evaluateArgs(arg,xValue,intermediateResults,3);
    if(intermediateResults.size()==0) return NAN;
    if(intermediateResults.size()<2) return intermediateResults.at(0);
    if(intermediateResults.size()==2) intermediateResults.push_back(0.5);
    T maxArg = intermediateResults.at(0);
    if(intermediateResults.at(1)>maxArg) maxArg=intermediateResults.at(1);
    T enumerator = intermediateResults.at(2)-abs(intermediateResults.at(0)-intermediateResults.at(1));
    if(0>enumerator) enumerator=0;
    return maxArg+(pow(enumerator,2)/(4*intermediateResults.at(2)));
    // return (intermediateResults.at(0)+intermediateResults.at(1)+sqrt(pow((intermediateResults.at(0)-intermediateResults.at(1)),2)+intermediateResults.at(2)))/2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateSmin(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    evaluateArgs(arg,xValue,intermediateResults,3);
    if(intermediateResults.size()==0) return NAN;
    if(intermediateResults.size()<2) return intermediateResults.at(0);
    if(intermediateResults.size()==2) intermediateResults.push_back(0.5);
    T min = intermediateResults.at(0);
    if(intermediateResults.at(1)<min) min=intermediateResults.at(1);
    T enumerator = intermediateResults.at(2)-abs(intermediateResults.at(0)-intermediateResults.at(1));
    if(0>enumerator) enumerator=0;
    return min-(pow(enumerator,2)/(4*intermediateResults.at(2)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateMix(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    evaluateArgs(arg,xValue,intermediateResults,3);
    if(intermediateResults.size()==0) return NAN;
    else if(intermediateResults.size()<2) return intermediateResults.at(0);
    else if(intermediateResults.at(2)>=1) return intermediateResults.at(1);
    else if(intermediateResults.at(2)<=0) return intermediateResults.at(0);
    
    else return intermediateResults.at(0)*(1-intermediateResults.at(2))+intermediateResults.at(1)*intermediateResults.at(2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateSabs(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    evaluateArgs(arg,xValue,intermediateResults,2);
    if(intermediateResults.size()==0) return NAN;
    if(intermediateResults.size()<2) intermediateResults.emplace_back(0.1); // Default argument
    T enumerator = intermediateResults.at(1)-abs(intermediateResults.at(0));
    if(enumerator<0) enumerator=0;
    return abs(intermediateResults.at(0))+(pow(enumerator,2)/(2*intermediateResults.at(1)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateIf(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    evaluateArgs(arg,xValue,intermediateResults,3);
    if(intermediateResults.size()==0) return NAN;
    if(intermediateResults.size()==1) return !(!intermediateResults.at(0));
    if(intermediateResults.size()==2)
    {
        if(intermediateResults.at(0)) return intermediateResults.at(1);
        else return 0;
    }
    else 
    {
        if(intermediateResults.at(0)) return intermediateResults.at(1);
        else return intermediateResults.at(2);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template <typename T>
T evaluateRoot(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    evaluateArgs(arg,xValue,intermediateResults,2);
    if(intermediateResults.size()==0) return NAN;
    if(intermediateResults.size()==1) intermediateResults.emplace_back(2); // Default argument

    // std::swap(intermediateResults.at(0),intermediateResults.at(1)); // My brain is too fried to change the code below. Don't kill me.
    
    Point frac {decimalToFraction(intermediateResults.at(1))};
    if(intermediateResults.at(0)==0) return NAN;
    if(abs(frac.x)!=INFINITY)
    {
        if(fmod(frac.x,2)==1 && fmod(frac.y,2)==0 && intermediateResults.at(0)<0)
        {
            return pow(-intermediateResults.at(0),1/intermediateResults.at(1));
        }
        else if(fmod(frac.x,2)==1 && fmod(frac.y,2)==1 && intermediateResults.at(0)<0)
        {
            return -pow(-intermediateResults.at(0),1/intermediateResults.at(1));
        }
    }
    return pow(intermediateResults.at(0), 1/intermediateResults.at(1));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateLog(Token &arg, const T xValue)
{
    std::vector<T> intermediateResults;
    std::string currentToken;
    evaluateArgs(arg,xValue,intermediateResults,2);
    if(intermediateResults.size()==0) return NAN;
    if(intermediateResults.size()==1) intermediateResults.emplace(intermediateResults.begin(),10); // Default argument
    return log(intermediateResults.at(1))/log(intermediateResults.at(0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateBinary(Token &numberStringLeft, Token &operation, Token &numberStringRight, const T xValue)
{
    T numberLeft{numberStringLeft.number(xValue)};
    T numberRight{numberStringRight.number(xValue)};

    if(operation.value()=="<") return numberLeft<numberRight;
    if(operation.value()==">") return numberLeft>numberRight;    
    if(operation.value()=="=") return numberLeft==numberRight;
    if(operation.value()==">=") return numberLeft>=numberRight;
    if(operation.value()=="<=") return numberLeft<=numberRight;  
    if(operation.value()=="=!") return numberLeft!=numberRight;
    if(operation.value()=="OR") return numberLeft||numberRight;
    if(operation.value()=="AND") return numberLeft&&numberRight;
    if(operation.value()=="XOR") return (!numberLeft)!=(!numberRight);
    if(operation.value()=="NOR") return (numberLeft==0)&&(numberRight==0);
    if(operation.value()=="AROUND") return abs(numberLeft-numberRight)<=globals::aroundLeniency.aroundTruthinessLeniency;
    

    if(operation.value()=="+") return numberLeft+numberRight;
    else if(operation.value()=="h*") return numberLeft*numberRight;
    else if(operation.value()=="*") return numberLeft*numberRight;
    else if(operation.value()=="/") return numberLeft/numberRight;
    else if(operation.value()=="^" || operation.value()=="**") 
    {
        Point frac {decimalToFraction(numberRight)};
        if(abs(frac.x)!=INFINITY)
        {
            if(fmod(frac.y,2)==1 && fmod(frac.x,2)==0 && numberLeft<0)
            {
                return pow(-numberLeft,numberRight);
            }
            else if(fmod(frac.y,2)==1 && fmod(frac.x,2)==1 && numberLeft<0)
            {
                return -pow(-numberLeft,numberRight);
            }
        }
        return pow(numberLeft, numberRight);
    }
    else if(operation.value()=="mod" || operation.value()=="%") return fmod(numberLeft,numberRight);
    else if(operation.value()=="nPk") return (tgamma(numberLeft+1)/tgamma(numberLeft-numberRight+1));
    else if(operation.value()=="nCk") return (tgamma(numberLeft+1)/(tgamma(numberRight+1)*tgamma(numberLeft-numberRight+1)));

    std::unreachable();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateUnary(Token &numberString, Token &operation, const T xValue)
{
    T number=numberString.number(xValue);
    T result{1};
    Token e{"e"};
    if(operation.value()=="-") return -number;

    if(operation.value()=="sign")
    {
        if(number==0) return 0;
        if(number>0) return 1;
        if(number<0) return -1;
    }

    if(operation.value()=="sqrt") return sqrt(number);
    if(operation.value()=="cbrt") return cbrt(number);
    if(operation.value()=="qtrt") return pow(number,0.25);
    
    if(operation.value()=="sinc")
    {
        if(number!=0) return sin(number)/number;
        else return 1;
    }

    if(operation.value()=="ReLU") return (number+abs(number))/2;

    if(operation.value()=="sstep")
    {
        if(number>1) return 1;
        else if(number<0) return 0;
        return pow(number,2)*(3-2*number);
    }
    
    if(operation.value()=="sat") return (1+abs(number)-abs(number-1))/2;

    if(operation.value()=="sin") return sin(number);
    if(operation.value()=="cos") return cos(number);
    if(operation.value()=="tan") return tan(number);

    if(operation.value()=="sec") return 1/cos(number);
    if(operation.value()=="csc") return 1/sin(number);
    if(operation.value()=="cot") return 1/tan(number);

    if(operation.value()=="asec") return acos(1/number);
    if(operation.value()=="acsc") return asin(1/number);
    if(operation.value()=="acot") return atan(1/number);

    if(operation.value()=="sinh") return sinh(number);
    if(operation.value()=="cosh") return cosh(number);
    if(operation.value()=="tanh") return tanh(number);

    if(operation.value()=="asinh") return asinh(number);
    if(operation.value()=="acosh") return acosh(number);
    if(operation.value()=="atanh") return atanh(number);

    if(operation.value()=="asech") return acosh(1/number);
    if(operation.value()=="acsch") return asinh(1/number);
    if(operation.value()=="acoth") return atanh(1/number);

    if(operation.value()=="sech") return 1/cosh(number);
    if(operation.value()=="csch") return 1/sinh(number);
    if(operation.value()=="coth") return 1/tanh(number);

    if(operation.value()=="asin") return asin(number);
    if(operation.value()=="acos") return acos(number);
    if(operation.value()=="atan") return atan(number);

    if(operation.value()=="round") return round(number);
    if(operation.value()=="floor") return floor(number);
    if(operation.value()=="ceil") return ceil(number);
    if(operation.value()=="abs") return abs(number);
    if(operation.value()=="ln") return log(number);
    if(operation.value()=="log10") return log10(number);
    if(operation.value()=="exp") return pow(e.number(xValue),number);

    if(operation.value()=="!")
    {
        return tgamma(number+1);
    }

    if(operation.value()=="!!")
    {
        if(number<0) return NAN;
        number=round(number);

        if constexpr(std::is_same<T,float>())
        {
            for(T i{std::fmod(number, 2.f)+2}; i<number+1; i+=2)
            {
                if(number>19572801.5) return INFINITY;
                if(number==0) return 1.0;
                result*=i;
            }
        }
        else for(T i{fmod(number, 2)+2}; i<number+1; i+=2)
        {
            if(number>19572801.5) return INFINITY;
            if(number==0) return 1.0;
            result*=i;
        }
        if(number<=3) return number;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isNumber(const std::string &input)
{
    if(input=="") return false;
    for(size_t i{}; i<input.length(); i++) if((input.at(i)<'0' || input.at(i)>'9') && 
                                               input.at(i)!='e' && 
                                               input.at(i)!='.' &&
                                               input.at(i)!='+' &&
                                               input.at(i)!='-') return false;
    uint dotCount{};
    uint eCount{};
    bool seenMinus{};

    if(input=="inf") return true;
    if(input=="-inf") return true;
    if(input=="nan") return true;
    if(input=="-nan") return true;
    if(input=="e") return false;
    if(input=="-") return false;
    for(size_t i{}; i<input.length(); i++)
    {
        if((seenMinus && input.at(i)=='-')||(i>0 && input.at(i)=='-' && input.at(i-1)!='e')) return false;
        if(input.at(0)=='-' && i==0 && input.length()>1)
        {
            seenMinus=true;
            continue;
        }
        if(input.at(i)=='e') 
        {
            if(i+2<input.length() && input.at(i)=='e' && (input.at(i+1)=='+' || input.at(i+1)=='-') && std::isdigit(input.at(i+2))) i+=2;
            eCount++;
        }
        if(input.at(i)=='.')
        {
            dotCount++;
            if(eCount) return false;
            if(dotCount>1) return false;
        }
        if(eCount>1) return false;

        if(!isNumberPart(input.at(i))) return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isNumberPart(const char input)
{
    return (input>='0' && input<='9') || input=='.' || input=='e';
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool replaceMacros(std::string &equation)
{
    if(globals::userMacros.size()==0) return false;
    for(size_t i{}; i<globals::userMacros.size(); i++)
    {
        for(int j{}; j<equation.length(); j++)
        {
            if(equation.find(globals::userMacros.at(i).name,j)==j)
            {
                if(j>=3 && equation.find("set",j-3)==j-3)
                {
                    break;
                }
                equation.erase(j,globals::userMacros.at(i).name.length());
                equation.insert(j,globals::userMacros.at(i).value);
                i=0;
                j=-1;
            }
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


inline long long unguardedGcd(long long a, long long b)
{
    if(b==0) return a;
    else return unguardedGcd(b,a%b);
}

template <typename T>
Point decimalToFraction(T enumerator)
{
    std::string fraction;
    std::string number;
    std::ostringstream asOSStream;
    asOSStream.precision(15);
    asOSStream<<enumerator;
    number=asOSStream.str();
    T denominator{1};

    for(size_t i{}; enumerator!=round(enumerator); i++)
    {
        enumerator*=10;
        denominator*=10;
    }


    size_t length = number.substr(number.find('.')+1).length();
    std::string pattern=number.substr(number.find('.')+1,length/2-1);
    size_t patternInstancesFound{};
    for(size_t i{}; i<number.length() && pattern!="" && length>2; i++)
    {
        if(number.find(pattern,i)==i)
        {
            patternInstancesFound++;
            i+=pattern.length()-1;
        }
    }

    
    if(patternInstancesFound>1)
    {
        if constexpr (std::is_same<double, T>()) 
        {
            enumerator=std::stold(number.substr(0,number.find('.')));
        }
        else enumerator=static_cast<cpp_dec_float_100>(number.substr(0,number.find('.')));
        denominator=pow(10,pattern.length())-1;
        enumerator=(enumerator*denominator)+std::stoll(pattern);
    }
    long long enumeratori=static_cast<double>(enumerator);
    long long denominatori=static_cast<double>(denominator);
    long long gcf = unguardedGcd(enumeratori,denominatori);


    enumerator/=gcf;
    denominator/=gcf;

    return Point(static_cast<double>(enumerator),static_cast<double>(denominator));

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool addIdentifier(Variable newVariable)
{
    for(size_t i{}; i<globals::userMacros.size(); i++)
    {
        if(newVariable.name==globals::userMacros.at(i).name) 
        {
            std::cerr<<"Duplicate names are not permissible.\n\n";
            return true;
        }
    }
    for(size_t i{}; i<globals::userVariables.size(); i++)
    {
        if(newVariable.name==globals::userVariables.at(i).name) 
        {
            globals::userVariables.at(i).value=newVariable.value;
            std::sort(globals::userVariables.begin(), globals::userVariables.end(), sortVariablesByNameLength);
            return false;
        }
    }
    globals::userVariables.emplace_back(newVariable);
    std::sort(globals::userVariables.begin(), globals::userVariables.end(), sortVariablesByNameLength);
    return false;
}

inline bool addIdentifier(Macro newMacro)
{
    for(size_t i{}; i<globals::userVariables.size(); i++)
    {
        if(newMacro.name==globals::userVariables.at(i).name) 
        {
            std::cerr<<"\nDuplicate names are not permissible.\n";
            return true;
        }
    }
    for(size_t i{}; i<globals::userMacros.size(); i++)
    {
        if(newMacro.name==globals::userMacros.at(i).name) 
        {
            globals::userMacros.at(i).value=newMacro.value;
            std::sort(globals::userMacros.begin(), globals::userMacros.end(), sortMacroesByNameLength);
            return false;
        }
    }
    globals::userMacros.emplace_back(newMacro);
    std::sort(globals::userMacros.begin(), globals::userMacros.end(), sortMacroesByNameLength);
    return false;
}

/*
3+(pi/root(2+4,10-2))-25x

3: Number                                               -> NUMBER
+: BinaryOp                                             -> OPERATOR
(pi/root(2+4,10-2)): SubExpr                            -> SUBEXPR
    pi: Constant (Will later be replaced by Number)     -> CONSTANT
    /: BinaryOp                                         -> OPERATOR
    root(2+4,10-2) 
        2+4: RootArgLeft                                -> SUBEXPR
            2: Number                                   -> NUMBER
            +: BinaryOp                                 -> OPERATOR
            4: Number                                   -> NUMBER
        10-2: RootArgRight                              -> SUBEXPR
            10: Number                                  -> NUMBER
            -: UnaryMinus                               -> OPERATOR
            2: Number                                   -> NUMBER
-:UnaryOp (Will later be treated as +-)                 -> OPERATOR
25:Number                                               -> NUMBER
x:Variable (Will later be replaced by Number)           -> NUMBER

*/
/*
    Grammar: (Subexpr could also be variable or constant)
    NUMBER||SUBEXPR then SUBEXPR||UNARYOP
    NUMBER||SUBEXPR then BINARYOP then NUMBER||SUBEXPR
    ANY then SUBEXPR
    SUBEXPR then ANY
*/   
}