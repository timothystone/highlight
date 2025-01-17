/***************************************************************************
                          codegenerator.cpp  -  description
                             -------------------
    begin                : Die Jul 9 2002
    copyright            : (C) 2002-2019 by Andre Simon
    email                : a.simon@mailbox.org
 ***************************************************************************/


/*
This file is part of Highlight.

Highlight is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Highlight is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Highlight.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <climits>
#include <memory>
#include <boost/xpressive/xpressive_dynamic.hpp>

#include "codegenerator.h"

#include "htmlgenerator.h"
#include "xhtmlgenerator.h"
#include "rtfgenerator.h"
#include "latexgenerator.h"
#include "texgenerator.h"
#include "svggenerator.h"
#include "bbcodegenerator.h"
#include "pangogenerator.h"
#include "odtgenerator.h"
#include "astyle/astyle.h"
#include "astyle/ASStreamIterator.h"

#if !defined (QT)
#include "ansigenerator.h"
#include "xterm256generator.h"
#endif

namespace highlight
{
const unsigned int CodeGenerator::NUMBER_BUILTIN_STATES = highlight::KEYWORD;

const string CodeGenerator::STY_NAME_STD="std";
const string CodeGenerator::STY_NAME_STR="str";
const string CodeGenerator::STY_NAME_NUM="num";
const string CodeGenerator::STY_NAME_SLC="slc";
const string CodeGenerator::STY_NAME_COM="com";
const string CodeGenerator::STY_NAME_ESC="esc";
const string CodeGenerator::STY_NAME_DIR="ppc"; //preprocessor
const string CodeGenerator::STY_NAME_DST="pps"; //preprocessor string
const string CodeGenerator::STY_NAME_LIN="lin";
const string CodeGenerator::STY_NAME_SYM="opt"; //operator
const string CodeGenerator::STY_NAME_IPL="ipl"; //interpolation

vector<Diluculum::LuaFunction*> CodeGenerator::pluginChunks;


CodeGenerator * CodeGenerator::getInstance ( OutputType type )
{
    CodeGenerator* generator=NULL;
    switch ( type ) {
    case HTML:
        generator = new HtmlGenerator();
        break;
    case XHTML:
        generator = new XHtmlGenerator();
        break;
    case TEX:
        generator = new TexGenerator ();
        break;
    case LATEX:
        generator = new LatexGenerator();
        break;
    case RTF:
        generator = new RtfGenerator ();
        break;
    case SVG:
        generator = new SVGGenerator();
        break;
    case BBCODE:
        generator = new BBCodeGenerator();
        break;
    case PANGO:
        generator = new PangoGenerator();
        break;
    case ODTFLAT:
        generator = new ODTGenerator();
        break;
    case ESC_ANSI:
        generator = new AnsiGenerator();
        break;
    case ESC_XTERM256:
    case ESC_TRUECOLOR:
        generator = new Xterm256Generator();
        generator->setESCTrueColor(type==ESC_TRUECOLOR);
        break;
    default:
        break;
    }
    return generator;
}


CodeGenerator::CodeGenerator ( highlight::OutputType type )
    :currentSyntax(NULL),
     in ( NULL ),
     out ( NULL ),
     encoding ( "none" ),
     docTitle ( "Source file" ),
     maskWs ( false ),
     excludeWs ( false ),
     fragmentOutput ( false ),
     keepInjections( false ),
     showLineNumbers ( false ),
     lineNumberFillZeroes ( false ),
     printNewLines(true),
     omitVersionComment(false),
     isolateTags(false),
     baseFontSize("10"),
     lineNumber ( 0 ),
     lineNumberOffset ( 0 ),
     currentState ( _UNKNOWN ),
     currentKeywordClass ( 0 ),
     includeStyleDef ( false ),
     numberCurrentLine ( false ),
     lineIndex ( 0 ),
     lastLineLength( 0 ),
     syntaxChangeIndex(UINT_MAX),
     syntaxChangeLineNo(UINT_MAX),
     lineNumberWidth ( 5 ),
     startLineCnt( 1 ),
     startLineCntCurFile( 1 ),
     maxLineCnt ( UINT_MAX ),
     inputFilesCnt (0),
     processedFilesCnt (0),
     terminatingChar ( '\0' ),
     formatter ( NULL ),
     formattingEnabled ( false ),
     formattingPossible ( false ),
     validateInput ( false ),
     numberWrappedLines ( true ),
     noTrailingNewLine(false),
     resultOfHook(false),
     lineContainedTestCase(false),
     applySyntaxTestCase(false),
     keywordCase ( StringTools::CASE_UNCHANGED ),
     eolDelimiter ('\n'),
     outputType ( type )
{
}


CodeGenerator::~CodeGenerator()
{
    delete formatter;

    for ( map<string, SyntaxReader*>::iterator it=syntaxReaders.begin(); it!=syntaxReaders.end(); it++ ) {
        delete it->second;
    }
    
    for (unsigned int i=0; i<pluginChunks.size(); i++) {
        delete pluginChunks[i];
    }
    pluginChunks.clear();
}


bool CodeGenerator::initTheme ( const string& themePath )
{
    this->themePath=themePath;
    bool loadOK = docStyle.load ( themePath, outputType );
    initOutputTags();
    return loadOK;
}

const string& CodeGenerator::getStyleName()
{
    return themePath;
}

void CodeGenerator::setLineNumberWidth ( int w )
{
    lineNumberWidth=w;
}

int CodeGenerator::getLineNumberWidth()
{
    return lineNumberWidth;
}

void CodeGenerator::setPrintLineNumbers ( bool flag, unsigned int startCnt )
{
    showLineNumbers=flag;
    lineNumberOffset = startCnt-1;
}

bool CodeGenerator::getPrintLineNumbers()
{
    return showLineNumbers;
}

void CodeGenerator::setPrintZeroes ( bool flag )
{
    lineNumberFillZeroes=flag;
}

bool CodeGenerator::getPrintZeroes()
{
    return lineNumberFillZeroes;
}

void CodeGenerator::setIncludeStyle ( bool flag )
{
    includeStyleDef = flag;
}

void CodeGenerator::disableTrailingNL ( bool flag )
{
    noTrailingNewLine = flag;
}

void CodeGenerator::setStyleInputPath ( const string& path )
{
    styleInputPath = path;
}

void CodeGenerator::setStyleOutputPath ( const string& path )
{
    styleOutputPath = path;
}

void CodeGenerator::setPluginParameter ( const string& param )
{
    pluginParameter = param;
}

const string&  CodeGenerator::getStyleInputPath()
{
    return styleInputPath;
}

const string&  CodeGenerator::getStyleOutputPath()
{
    return styleOutputPath;
}

void CodeGenerator::setFragmentCode ( bool flag )
{
    fragmentOutput=flag;
}

bool CodeGenerator::getFragmentCode()
{
    return fragmentOutput;
}
void CodeGenerator::setKeepInjections ( bool flag )
{
    keepInjections=flag;
}

bool CodeGenerator::getKeepInjections()
{
    return keepInjections;
}
void CodeGenerator::setValidateInput ( bool flag )
{
    validateInput=flag;
}

bool CodeGenerator::getValidateInput()
{
    return validateInput;
}


void CodeGenerator::setNumberWrappedLines ( bool flag )
{
    numberWrappedLines=flag;
}

bool CodeGenerator::getNumberWrappedLines()
{
    return numberWrappedLines;
}

void CodeGenerator::setOmitVersionComment ( bool flag )
{
    omitVersionComment=flag;
}

bool CodeGenerator::getOmitVersionComment ()
{
    return omitVersionComment;
}

void CodeGenerator::setIsolateTags ( bool flag )
{
    isolateTags=flag;
}

bool CodeGenerator::getIsolateTags ()
{
    return isolateTags;
}

void CodeGenerator::setBaseFont ( const string& fontName )
{
    baseFont = fontName;
}

void CodeGenerator::setBaseFontSize ( const string& fontSize)
{
    baseFontSize = fontSize;
}

void CodeGenerator::setStartingNestedLang(const string &langName)
{
    embedLangStart = langName;
}

const string CodeGenerator::getBaseFont() const
{
    if ( !baseFont.empty() ) return baseFont;
    switch ( outputType ) {
    case HTML:
    case XHTML:
    case SVG:
        return "'Courier New',monospace";
        break;
    case LATEX:
        return "ttfamily";
        break;
    case TEX:
        return "tt";
        break;
    default:
        return "Courier New";
    }
}

const string CodeGenerator::getBaseFontSize()
{
    return baseFontSize;
}

void CodeGenerator::setTitle ( const string & title )
{
    if ( !title.empty() ) docTitle= title;
}

string CodeGenerator::getTitle()
{
    return docTitle;
}

void CodeGenerator::setEncoding ( const string& encodingName )
{
    encoding = encodingName;
}

bool CodeGenerator::formattingDisabled()
{
    return !formattingEnabled;
}

void CodeGenerator::setStartingInputLine ( unsigned int begin )
{
    startLineCnt = startLineCntCurFile = begin;
}

void CodeGenerator::setMaxInputLineCnt ( unsigned int cnt )
{
    maxLineCnt = cnt;
}

void CodeGenerator::setFilesCnt ( unsigned int cnt )
{
    inputFilesCnt = cnt;
}

bool CodeGenerator::formattingIsPossible()
{
    return formattingPossible;
}

void CodeGenerator::setPreformatting ( WrapMode lineWrappingStyle,
                                       unsigned int lineLength,
                                       int numberSpaces )
{
    bool enableWrap = lineWrappingStyle!=WRAP_DISABLED;
    bool replaceTabs = numberSpaces > 0;

    if ( enableWrap || replaceTabs ) {
        preFormatter.setWrap ( enableWrap );
        preFormatter.setWrapIndentBraces ( lineWrappingStyle==WRAP_DEFAULT );
        preFormatter.setWrapLineLength ( lineLength );
        preFormatter.setReplaceTabs ( replaceTabs );
        preFormatter.setNumberSpaces ( numberSpaces );
    }
}

void CodeGenerator::setKeyWordCase ( StringTools::KeywordCase keyCase )
{
    keywordCase = keyCase;
}

void CodeGenerator::setEOLDelimiter(char delim)
{
    eolDelimiter = delim;
}

void CodeGenerator::reset()
{
    lineIndex = 0;
    lineNumber = 0;
    line.clear();
    preFormatter.reset();
    inFile.clear();
    outFile.clear();
    embedLangDefPath.clear();
    printNewLines=true;
    syntaxChangeIndex = syntaxChangeLineNo = UINT_MAX;
    startLineCntCurFile = startLineCnt;
    applySyntaxTestCase=lineContainedTestCase=false;
}

string CodeGenerator::getThemeInitError()
{
    return  docStyle.getErrorMessage();
}

string CodeGenerator::getPluginScriptError()
{
    return userScriptError;
}

string CodeGenerator::getSyntaxRegexError()
{
    return (currentSyntax)? currentSyntax->getFailedRegex(): "syntax undef";
}
string CodeGenerator::getSyntaxLuaError()
{
    return (currentSyntax)? currentSyntax->getLuaErrorText(): "syntax undef";

}
string CodeGenerator::getSyntaxDescription()
{
    return (currentSyntax)? currentSyntax->getDescription(): "syntax undef";

}
string CodeGenerator::getThemeDescription()
{
    return docStyle.getDescription();
}

string CodeGenerator::getSyntaxCatDescription(){
    return (currentSyntax)? currentSyntax->getCategoryDescription(): "";
}

string CodeGenerator::getThemeCatDescription(){
    return docStyle.getCategoryDescription();
}

unsigned int CodeGenerator::getLineNumber()
{
    return lineNumber;
}

bool CodeGenerator::readNewLine ( string &newLine )
{
    bool eof=false;
    
    if ( lineIndex ) terminatingChar=newLine[lineIndex-1];
    
    while (!eof && startLineCntCurFile>0) {
        if ( formattingPossible && formattingEnabled ) {
            eof=!formatter->hasMoreLines();
            if ( !eof ) {
                newLine = formatter->nextLine();
            }
        } else {
            eof = ! getline ( *in, newLine, eolDelimiter );
        }
        --startLineCntCurFile;
    }
    startLineCntCurFile=1;
#ifndef _WIN32
    // drop CR of CRLF files
    if (!newLine.empty() && newLine[newLine.size() - 1] == '\r')
        newLine.erase(newLine.size() - 1);
#endif

    return eof || ( lineNumber == maxLineCnt );
}

void CodeGenerator::matchRegex ( const string &line, State skipState)
{
    regexGroups.clear();
    int matchBegin=0;
    int groupID=0;

    // cycle through all regex, save the start and ending indices of matches to report them later
    for ( unsigned int i=0; i<currentSyntax->getRegexElements().size(); i++ ) {
        RegexElement *regexElem = currentSyntax->getRegexElements() [i];

        if (regexElem->open == skipState) continue;
        
        boost::xpressive::sregex_iterator cur( line.begin(), line.end(), regexElem->rex );
        boost::xpressive::sregex_iterator end;

        for( ; cur != end; ++cur )  {
            groupID = ( regexElem->capturingGroup<0 ) ? cur->size()-1 : regexElem->capturingGroup;
            matchBegin =  cur->position(groupID);
            regexGroups.insert (
                make_pair ( matchBegin + 1, ReGroup ( regexElem->open, cur->length(groupID), regexElem->kwClass, regexElem->langName ) ) );
        }
    }
}

unsigned char CodeGenerator::getInputChar()
{
    // end of line?
    if ( lineIndex == line.length() ) {
        bool eof=false;
        if ( preFormatter.isEnabled() ) {
            if ( !preFormatter.hasMoreLines() ) {
                eof=readNewLine ( line );
                preFormatter.setLine ( line );
                ++lineNumber;
                numberCurrentLine = true;
            } else {
                if(numberWrappedLines)
                    ++lineNumber;
                numberCurrentLine = numberWrappedLines;
            }

            line = preFormatter.getNextLine();
        } else {
            eof=readNewLine ( line );
            ++lineNumber;

            numberCurrentLine = true;
        }
        lastLineLength=lineIndex;
        lineIndex=0;
        
        if (!lineContainedTestCase && applySyntaxTestCase){
            stateTraceTest = stateTraceCurrent;
            stateTraceCurrent.clear();
        } 
        
        lineContainedTestCase=false;
            
        matchRegex ( line );
        stateTrace.clear();
        return ( eof ) ?'\0':'\n';
    }

    return line[lineIndex++];
}

/** changing this method requires regression testing with nested syntax files (HTML+PHP+JS+CSS, Coffeescript with block regex, Pas + ASM) 
    especially nested syntax in one line
 */
State CodeGenerator::getCurrentState (State oldState)
{
    unsigned char c='\0';

    if ( token.length() ==0 ) {
        c=getInputChar();
    } else {
        lineIndex-= ( token.length()-1 );
        c=token[0];
    }
    if ( c=='\n' ) {
        return _EOL;   // End of line
    }

    if ( c=='\0' ) {
        return _EOF;   // End of file
    }

    if ( c==' ' || c=='\t' ) {
        token= c;
        return _WS;
    }
    
    //TODO add control flag
    if ( applySyntaxTestCase && ( c=='^' || c=='<') && (oldState == ML_COMMENT || oldState==SL_COMMENT)  ) {
        token= c;
        return _TESTPOS;
    }
        
    // at this position the syntax change takes place
    if (lineIndex >= syntaxChangeIndex-1 || syntaxChangeLineNo < lineNumber){
        loadEmbeddedLang(embedLangDefPath);  // load new syntax                     
        matchRegex(line);                    // recognize new patterns in the (remaining) line
        syntaxChangeIndex = syntaxChangeLineNo = UINT_MAX;
    }

SKIP_EMBEDDED:
    
    // Test if a regular expression was found at the current position
    if ( !regexGroups.empty() ) {
        if ( regexGroups.count ( lineIndex ) ) {
            token = line.substr ( lineIndex-1, regexGroups[lineIndex].length );

            unsigned int oldIndex= lineIndex;
            if ( regexGroups[oldIndex].length>1 ) lineIndex+= regexGroups[oldIndex].length-1;

            if ( regexGroups[oldIndex].state==EMBEDDED_CODE_BEGIN ) {
                //do not handle a nested section if the syntax is marked as "sealed" 
                if (embedLangDefPath.length()==0 || currentSyntax->allowsInnerSection(embedLangDefPath) ) {
                    embedLangDefPath = currentSyntax->getNewPath(regexGroups[oldIndex].name);
                    //remember position 
                    syntaxChangeIndex = lineIndex+2;
                    syntaxChangeLineNo = lineNumber;
                }
                
                // repeat parsing of this line without nested state recognition to highlight opening delimiter in the host syntax
                matchRegex(line, EMBEDDED_CODE_BEGIN);
                lineIndex = oldIndex;
                goto SKIP_EMBEDDED; // this is how it should be done
            }

            if ( regexGroups[oldIndex].state==IDENTIFIER_BEGIN || regexGroups[oldIndex].state==KEYWORD ) {
                string reservedWord= ( currentSyntax->isIgnoreCase() ) ? StringTools::change_case ( token ) :token;
                currentKeywordClass=currentSyntax->isKeyword ( reservedWord ); //check in lists (no regex)
                
                if ( !currentKeywordClass && regexGroups[oldIndex].state==KEYWORD ){
                    currentKeywordClass = regexGroups[oldIndex].kwClass;
                }
                return validateState(( currentKeywordClass ) ? KEYWORD : STANDARD, oldState );
            } else {
                return validateState(regexGroups[oldIndex].state, oldState);
            }
        }
    }

    // Character not referring to any state
    token = c;
    return STANDARD;
}

State CodeGenerator::validateState(State newState, State oldState)
{

    if (currentSyntax->getValidateStateChangeFct()) {
        Diluculum::LuaValueList params;
        params.push_back(Diluculum::LuaValue(oldState));
        params.push_back(Diluculum::LuaValue(newState));
        params.push_back(Diluculum::LuaValue(token));
        params.push_back(Diluculum::LuaValue(getCurrentKeywordClassId()) );
        params.push_back(Diluculum::LuaValue(lineNumber) );
        params.push_back(Diluculum::LuaValue(lineIndex-(unsigned int)token.length()) );

        Diluculum::LuaValueList res=
            currentSyntax->getLuaState()->call ( *currentSyntax->getValidateStateChangeFct(),
                    params,"getValidateStateChangeFct call")  ;

        resultOfHook = res.size()>=1;
        if (resultOfHook) {
            State validatedState = (State)res[0].asInteger();
            if ( validatedState== _REJECT) {
                // proceed using only the first character of the token
                // TODO evaluate if token clear would be better
                if (res.size()==1) { 
                    lineIndex -= (token.length() -1);
                    token=token.substr(0, 1);
                }
                
                //experimental for slim.lang: evaluate second return arg after _REJECT
                if (res.size()>=2) {
                    lineIndex -= (token.length() );
                    token.clear();
                    return (State)res[1].asInteger();
                }
                return oldState;
            }
            stateTrace.push_back(validatedState);
            if (stateTrace.size()>200) stateTrace.erase(stateTrace.begin(), stateTrace.begin() + 100 );
            return validatedState;
        }
    }
    resultOfHook  = false;
    stateTrace.push_back(newState);
    if (stateTrace.size()>200) stateTrace.erase(stateTrace.begin(), stateTrace.begin() + 100 );    
    return newState;
}


unsigned int CodeGenerator::getCurrentKeywordClassId(){
    unsigned int kwClassId=0;

    // this vector contains the defined keyword classes, and currentKeywordClass is its index:
    vector<string> kwClasses=currentSyntax->getKeywordClasses();
    if (currentKeywordClass && currentKeywordClass<=kwClasses.size()) {
        string kwClassName=kwClasses[currentKeywordClass-1];
        if (kwClassName.size()==3)
            kwClassId = kwClassName[2] - 'a' + 1;
    }
    return kwClassId;
}

//it is faster to pass ostream reference
void CodeGenerator::maskString ( ostream& ss, const string & s )
{
    for ( unsigned int i=0; i< s.length(); i++ ) {
        ss << maskCharacter ( s[i] );

        if (applySyntaxTestCase) {
            PositionState ps(currentState, getCurrentKeywordClassId(), false);
            stateTraceCurrent.push_back(ps);
            
            if (stateTraceCurrent.size()>200) 
                stateTraceCurrent.erase(stateTraceCurrent.begin(), stateTraceCurrent.begin() + 100 ); 
        }
    }
}


Diluculum::LuaValueList CodeGenerator::callDecorateFct(const string&token)
{
    Diluculum::LuaValueList params;
    params.push_back(Diluculum::LuaValue(token));
    params.push_back(Diluculum::LuaValue(currentState));
    params.push_back(Diluculum::LuaValue(currentKeywordClass));
    string trace(";");
    if (stateTrace.size()>1){
        for (size_t i=0; i<stateTrace.size()-1;i++){
            trace += std::to_string (stateTrace[i]);
            trace += ";";
        }
    }
    
    //std::cerr <<"TRC1: "<<trace<<"\n";
    
    params.push_back(Diluculum::LuaValue(trace));

    return currentSyntax->getLuaState()->call ( *currentSyntax->getDecorateFct(),
            params,"getDecorateFct call")  ;
}

void CodeGenerator::printMaskedToken (bool flushWhiteSpace, StringTools::KeywordCase tcase )
{
    if ( flushWhiteSpace )
        flushWs(1);
    string caseToken = StringTools::change_case ( token, tcase );
    if (currentSyntax->getDecorateFct()) {

        Diluculum::LuaValueList res=callDecorateFct(caseToken);
        if (res.size()==1) {
            *out<<res[0].asString();
        } else {
            maskString ( *out, caseToken );
        }
    } else {
        maskString ( *out, caseToken );
    }

    token.clear();
}

bool CodeGenerator::styleFound()
{
    return docStyle.found();
}

bool CodeGenerator::printIndexFile ( const vector<string> &fileList,
                                     const string &outPath )
{
    return true;
}

bool CodeGenerator::initIndentationScheme ( const string &indentScheme )
{

    if ( formatter!=NULL ) {
        return true;
    }

    if ( !indentScheme.size() ) return false;

    formatter=new astyle::ASFormatter();

    if ( indentScheme=="allman" || indentScheme=="bsd" || indentScheme=="ansi" ) {
        formatter->setFormattingStyle ( astyle::STYLE_ALLMAN );
    } else if ( indentScheme=="kr"||indentScheme=="k&r"||indentScheme=="k/r" ) {
        formatter->setFormattingStyle ( astyle::STYLE_KR );
    } else if ( indentScheme=="java" ) {
        formatter->setFormattingStyle ( astyle::STYLE_JAVA );
    } else if ( indentScheme=="stroustrup" ) {
        formatter->setFormattingStyle ( astyle::STYLE_STROUSTRUP );
    } else if ( indentScheme=="whitesmith" ) {
        formatter->setFormattingStyle ( astyle::STYLE_WHITESMITH );
    } else if ( indentScheme=="banner" || indentScheme=="ratliff") {
        formatter->setFormattingStyle ( astyle::STYLE_RATLIFF );
    } else if ( indentScheme=="gnu" ) {
        formatter->setFormattingStyle ( astyle::STYLE_GNU );
    } else if ( indentScheme=="linux" ) {
        formatter->setFormattingStyle ( astyle::STYLE_LINUX );
    } else if ( indentScheme=="horstmann" ) {
        formatter->setFormattingStyle ( astyle::STYLE_HORSTMANN );
    } else if ( indentScheme=="otbs" ||  indentScheme=="1tbs") {
        formatter->setFormattingStyle ( astyle::STYLE_1TBS );
    } else if ( indentScheme=="google") {
        formatter->setFormattingStyle ( astyle::STYLE_GOOGLE );
    } else if ( indentScheme=="pico" ||  indentScheme=="a11") {
        formatter->setFormattingStyle ( astyle::STYLE_PICO );
    } else if ( indentScheme=="lisp" ||  indentScheme=="python"||  indentScheme=="a12") {
        formatter->setFormattingStyle ( astyle::STYLE_LISP );
    } else if ( indentScheme=="vtk") {
        formatter->setFormattingStyle ( astyle::STYLE_VTK );
    } else if ( indentScheme=="mozilla") {
        formatter->setFormattingStyle ( astyle::STYLE_MOZILLA );
    } else if ( indentScheme=="webkit") {
        formatter->setFormattingStyle ( astyle::STYLE_WEBKIT );
    } else if ( indentScheme!="user" ){
        return false;
    }
    return formattingEnabled=true;
}


/*Helper functions for astyle option parsing*/
string CodeGenerator::getParam(const string& arg, const char* op)
{
	return arg.substr(strlen(op));
}

string CodeGenerator::getParam(const string& arg, const char* op1, const char* op2)
{
	return isParamOption(arg, op1) ? getParam(arg, op1) : getParam(arg, op2);
}

bool CodeGenerator::isOption(const string& arg, const char* op)
{
	return arg.compare(op) == 0;
}

bool CodeGenerator::isOption(const string& arg, const char* op1, const char* op2)
{
	return (isOption(arg, op1) || isOption(arg, op2));
}

bool CodeGenerator::isParamOption(const string& arg, const char* option)
{
	bool retVal = arg.compare(0, strlen(option), option) == 0;
	// if comparing for short option, 2nd char of arg must be numeric
	if (retVal && strlen(option) == 1 && arg.length() > 1)
		if (!isdigit((unsigned char) arg[1]))
			retVal = false;
	return retVal;
}

bool CodeGenerator::isParamOption(const string& arg, const char* option1, const char* option2)
{
	return isParamOption(arg, option1) || isParamOption(arg, option2);
}

//apply the same options as astyle
void CodeGenerator::setIndentationOptions (const vector<string>& options){
    if (formatter) {
        string arg;
        for (unsigned int i=0; i<options.size(); i++) {
            arg=options[i];
            
            if (isOption(arg, "mode=cs"))
            {
                formatter->setSharpStyle();
                formatter->setModeManuallySet(true);
            }
            else if (isOption(arg, "mode=c"))
            {
                formatter->setCStyle();
                formatter->setModeManuallySet(true);
            }
            else if (isOption(arg, "mode=java"))
            {
                formatter->setJavaStyle();
                formatter->setModeManuallySet(true);
            }
            else if (isParamOption(arg, "t", "indent=tab="))
            {
                int spaceNum = 4;
                string spaceNumParam = getParam(arg, "t", "indent=tab=");
                if (spaceNumParam.length() > 0)
                    spaceNum = atoi(spaceNumParam.c_str());
                if (spaceNum >= 2 && spaceNum <= 20)
                    formatter->setTabIndentation(spaceNum, false);
            }
            else if (isOption(arg, "indent=tab"))
            {
                formatter->setTabIndentation(4);
            }
            else if (isParamOption(arg, "T", "indent=force-tab="))
            {
                int spaceNum = 4;
                string spaceNumParam = getParam(arg, "T", "indent=force-tab=");
                if (spaceNumParam.length() > 0)
                    spaceNum = atoi(spaceNumParam.c_str());
                if (spaceNum >= 2 && spaceNum <= 20)
                    formatter->setTabIndentation(spaceNum, true);
            }
            else if (isOption(arg, "indent=force-tab"))
            {
                formatter->setTabIndentation(4, true);
            }
            else if (isParamOption(arg, "xT", "indent=force-tab-x="))
            {
                int tabNum = 8;
                string tabNumParam = getParam(arg, "xT", "indent=force-tab-x=");
                if (tabNumParam.length() > 0)
                    tabNum = atoi(tabNumParam.c_str());
                if (tabNum >= 2 && tabNum <= 20)
                    formatter->setForceTabXIndentation(tabNum);
                
            }
            else if (isOption(arg, "indent=force-tab-x"))
            {
                formatter->setForceTabXIndentation(8);
            }
            else if (isParamOption(arg, "s", "indent=spaces="))
            {
                int spaceNum = 4;
                string spaceNumParam = getParam(arg, "s", "indent=spaces=");
                if (spaceNumParam.length() > 0)
                    spaceNum = atoi(spaceNumParam.c_str());
                if (spaceNum >= 2 && spaceNum <= 20)
                    formatter->setSpaceIndentation(spaceNum);
            }
            else if (isOption(arg, "indent=spaces"))
            {
                formatter->setSpaceIndentation(4);
            }
            else if (isParamOption(arg, "xt", "indent-continuation="))
            {
                int contIndent = 1;
                string contIndentParam = getParam(arg, "xt", "indent-continuation=");
                if (contIndentParam.length() > 0)
                    contIndent = atoi(contIndentParam.c_str());
                if (contIndent > 0 && contIndent < 5)
                    formatter->setContinuationIndentation(contIndent);
            }
            else if (isParamOption(arg, "m", "min-conditional-indent="))
            {
                int minIndent = astyle::MINCOND_TWO;
                string minIndentParam = getParam(arg, "m", "min-conditional-indent=");
                if (minIndentParam.length() > 0)
                    minIndent = atoi(minIndentParam.c_str());
                if (minIndent < astyle::MINCOND_END)
                    formatter->setMinConditionalIndentOption(minIndent);
            }
            else if (isParamOption(arg, "M", "max-continuation-indent="))
            {
                int maxIndent = 40;
                string maxIndentParam = getParam(arg, "M", "max-continuation-indent=");
                if (maxIndentParam.length() > 0)
                    maxIndent = atoi(maxIndentParam.c_str());
                if (maxIndent >= 40 && maxIndent <= 120)
                    formatter->setMaxContinuationIndentLength(maxIndent);
            }
            else if (isOption(arg, "N", "indent-namespaces"))
            {
                formatter->setNamespaceIndent(true);
            }
            else if (isOption(arg, "C", "indent-classes"))
            {
                formatter->setClassIndent(true);
            }
            else if (isOption(arg, "xG", "indent-modifiers"))
            {
                formatter->setModifierIndent(true);
            }
            else if (isOption(arg, "S", "indent-switches"))
            {
                formatter->setSwitchIndent(true);
            }
            else if (isOption(arg, "K", "indent-cases"))
            {
                formatter->setCaseIndent(true);
            }
            else if (isOption(arg, "xU", "indent-after-parens"))
            {
                formatter->setAfterParenIndent(true);
            }
            else if (isOption(arg, "L", "indent-labels"))
            {
                formatter->setLabelIndent(true);
            }
            else if (isOption(arg, "xW", "indent-preproc-block"))
            {
                formatter->setPreprocBlockIndent(true);
            }
            else if (isOption(arg, "w", "indent-preproc-define"))
            {
                formatter->setPreprocDefineIndent(true);
            }
            else if (isOption(arg, "xw", "indent-preproc-cond"))
            {
                formatter->setPreprocConditionalIndent(true);
            }
            else if (isOption(arg, "y", "break-closing-braces"))
            {
                formatter->setBreakClosingHeaderBracesMode(true);
            }
            else if (isOption(arg, "O", "keep-one-line-blocks"))
            {
                formatter->setBreakOneLineBlocksMode(false);
            }
            else if (isOption(arg, "o", "keep-one-line-statements"))
            {
                formatter->setBreakOneLineStatementsMode(false);
            }
            else if (isOption(arg, "P", "pad-paren"))
            {
                formatter->setParensOutsidePaddingMode(true);
                formatter->setParensInsidePaddingMode(true);
            }
            else if (isOption(arg, "d", "pad-paren-out"))
            {
                formatter->setParensOutsidePaddingMode(true);
            }
            else if (isOption(arg, "xd", "pad-first-paren-out"))
            {
                formatter->setParensFirstPaddingMode(true);
            }
            else if (isOption(arg, "D", "pad-paren-in"))
            {
                formatter->setParensInsidePaddingMode(true);
            }
            else if (isOption(arg, "H", "pad-header"))
            {
                formatter->setParensHeaderPaddingMode(true);
            }
            else if (isOption(arg, "U", "unpad-paren"))
            {
                formatter->setParensUnPaddingMode(true);
            }
            else if (isOption(arg, "p", "pad-oper"))
            {
                formatter->setOperatorPaddingMode(true);
            }
            else if (isOption(arg, "xg", "pad-comma"))
            {
                formatter->setCommaPaddingMode(true);
            }
            else if (isOption(arg, "xe", "delete-empty-lines"))
            {
                formatter->setDeleteEmptyLinesMode(true);
            }
            else if (isOption(arg, "E", "fill-empty-lines"))
            {
                formatter->setEmptyLineFill(true);
            }
            else if (isOption(arg, "c", "convert-tabs"))
            {
                formatter->setTabSpaceConversionMode(true);
            }
            else if (isOption(arg, "xy", "close-templates"))
            {
                formatter->setCloseTemplatesMode(true);
            }
            else if (isOption(arg, "F", "break-blocks=all"))
            {
                formatter->setBreakBlocksMode(true);
                formatter->setBreakClosingHeaderBlocksMode(true);
            }
            else if (isOption(arg, "f", "break-blocks"))
            {
                formatter->setBreakBlocksMode(true);
            }
            else if (isOption(arg, "e", "break-elseifs"))
            {
                formatter->setBreakElseIfsMode(true);
            }
            else if (isOption(arg, "xb", "break-one-line-headers"))
            {
                formatter->setBreakOneLineHeadersMode(true);
            }
            else if (isOption(arg, "j", "add-braces"))
            {
                formatter->setAddBracesMode(true);
            }
            else if (isOption(arg, "J", "add-one-line-braces"))
            {
                formatter->setAddOneLineBracesMode(true);
            }
            else if (isOption(arg, "xj", "remove-braces"))
            {
                formatter->setRemoveBracesMode(true);
            }
            else if (isOption(arg, "Y", "indent-col1-comments"))
            {
                formatter->setIndentCol1CommentsMode(true);
            }
            else if (isOption(arg, "align-pointer=type"))
            {
                formatter->setPointerAlignment(astyle::PTR_ALIGN_TYPE);
            }
            else if (isOption(arg, "align-pointer=middle"))
            {
                formatter->setPointerAlignment(astyle::PTR_ALIGN_MIDDLE);
            }
            else if (isOption(arg, "align-pointer=name"))
            {
                formatter->setPointerAlignment(astyle::PTR_ALIGN_NAME);
            }
            else if (isParamOption(arg, "k"))
            {
                int align = 0;
                string styleParam = getParam(arg, "k");
                if (styleParam.length() > 0)
                    align = atoi(styleParam.c_str());
                if (align == 1)
                    formatter->setPointerAlignment(astyle::PTR_ALIGN_TYPE);
                else if (align == 2)
                    formatter->setPointerAlignment(astyle::PTR_ALIGN_MIDDLE);
                else if (align == 3)
                    formatter->setPointerAlignment(astyle::PTR_ALIGN_NAME);
            }
            else if (isOption(arg, "align-reference=none"))
            {
                formatter->setReferenceAlignment(astyle::REF_ALIGN_NONE);
            }
            else if (isOption(arg, "align-reference=type"))
            {
                formatter->setReferenceAlignment(astyle::REF_ALIGN_TYPE);
            }
            else if (isOption(arg, "align-reference=middle"))
            {
                formatter->setReferenceAlignment(astyle::REF_ALIGN_MIDDLE);
            }
            else if (isOption(arg, "align-reference=name"))
            {
                formatter->setReferenceAlignment(astyle::REF_ALIGN_NAME);
            }
            else if (isParamOption(arg, "W"))
            {
                int align = 0;
                string styleParam = getParam(arg, "W");
                if (styleParam.length() > 0)
                    align = atoi(styleParam.c_str());
                if (align == 0)
                    formatter->setReferenceAlignment(astyle::REF_ALIGN_NONE);
                else if (align == 1)
                    formatter->setReferenceAlignment(astyle::REF_ALIGN_TYPE);
                else if (align == 2)
                    formatter->setReferenceAlignment(astyle::REF_ALIGN_MIDDLE);
                else if (align == 3)
                    formatter->setReferenceAlignment(astyle::REF_ALIGN_NAME);
            }
            else if (isParamOption(arg, "max-code-length="))
            {
                int maxLength = 50;
                string maxLengthParam = getParam(arg, "max-code-length=");
                if (maxLengthParam.length() > 0)
                    maxLength = atoi(maxLengthParam.c_str());
                if (maxLength >= 50 && maxLength<= 200)
                    formatter->setMaxCodeLength(maxLength);
            }
            else if (isParamOption(arg, "xC"))
            {
                int maxLength = 50;
                string maxLengthParam = getParam(arg, "xC");
                if (maxLengthParam.length() > 0)
                    maxLength = atoi(maxLengthParam.c_str());
                if (maxLength > 0 && maxLength<= 200)
                    formatter->setMaxCodeLength(maxLength);
            }
            else if (isOption(arg, "xL", "break-after-logical"))
            {
                formatter->setBreakAfterMode(true);
            }
            else if (isOption(arg, "xc", "attach-classes"))
            {
                formatter->setAttachClass(true);
            }
            else if (isOption(arg, "xV", "attach-closing-while"))
            {
                formatter->setAttachClosingWhile(true);
            }
            else if (isOption(arg, "xk", "attach-extern-c"))
            {
                formatter->setAttachExternC(true);
            }
            else if (isOption(arg, "xn", "attach-namespaces"))
            {
                formatter->setAttachNamespace(true);
            }
            else if (isOption(arg, "xl", "attach-inlines"))
            {
                formatter->setAttachInline(true);
            }
            else if (isOption(arg, "xp", "remove-comment-prefix"))
            {
                formatter->setStripCommentPrefix(true);
            }
            else if (isOption(arg, "xB", "break-return-type"))
            {
                formatter->setBreakReturnType(true);
            }
            else if (isOption(arg, "xD", "break-return-type-decl"))
            {
                formatter->setBreakReturnTypeDecl(true);
            }
            else if (isOption(arg, "xf", "attach-return-type"))
            {
                formatter->setAttachReturnType(true);
            }
            else if (isOption(arg, "xh", "attach-return-type-decl"))
            {
                formatter->setAttachReturnTypeDecl(true);
            }
            // Objective-C options
            else if (isOption(arg, "xQ", "pad-method-prefix"))
            {
                formatter->setMethodPrefixPaddingMode(true);
            }
            else if (isOption(arg, "xR", "unpad-method-prefix"))
            {
                formatter->setMethodPrefixUnPaddingMode(true);
            }
            else if (isOption(arg, "xq", "pad-return-type"))
            {
                formatter->setReturnTypePaddingMode(true);
            }
            else if (isOption(arg, "xr", "unpad-return-type"))
            {
                formatter->setReturnTypeUnPaddingMode(true);
            }
            else if (isOption(arg, "xS", "pad-param-type"))
            {
                formatter->setParamTypePaddingMode(true);
            }
            else if (isOption(arg, "xs", "unpad-param-type"))
            {
                formatter->setParamTypeUnPaddingMode(true);
            }
            else if (isOption(arg, "xM", "align-method-colon"))
            {
                formatter->setAlignMethodColon(true);
            }
            else if (isOption(arg, "xP0", "pad-method-colon=none"))
            {
                formatter->setObjCColonPaddingMode(astyle::COLON_PAD_NONE);
            }
            else if (isOption(arg, "xP1", "pad-method-colon=all"))
            {
                formatter->setObjCColonPaddingMode(astyle::COLON_PAD_ALL);
            }
            else if (isOption(arg, "xP2", "pad-method-colon=after"))
            {
                formatter->setObjCColonPaddingMode(astyle::COLON_PAD_AFTER);
            }
            else if (isOption(arg, "xP3", "pad-method-colon=before"))
            {
                formatter->setObjCColonPaddingMode(astyle::COLON_PAD_BEFORE);
            }
        }
    }
}

LoadResult CodeGenerator::loadLanguage ( const string& langDefPath, bool embedded )
{

    if (!embedded) {
        while (!nestedLangs.empty()) {
            nestedLangs.pop();
        }   
    }
    
    bool reloadNecessary= currentSyntax ? currentSyntax->needsReload ( langDefPath ): true;
    LoadResult result=LOAD_OK;

    if ( reloadNecessary ) {
        if (syntaxReaders.count(langDefPath)) {
            currentSyntax=syntaxReaders[langDefPath];
            result=LOAD_OK;
        } else {
            currentSyntax=new SyntaxReader();
            result=currentSyntax->load(langDefPath, pluginParameter, outputType);
            syntaxReaders[langDefPath]=currentSyntax;
        }

        if ( result==LOAD_OK ) {
            formattingPossible=currentSyntax->enableReformatting();

            if ( openTags.size() >NUMBER_BUILTIN_STATES ) {
                // remove dynamic keyword tag delimiters of the old language definition
                vector<string>::iterator keyStyleOpenBegin =
                    openTags.begin() + NUMBER_BUILTIN_STATES;
                vector<string>::iterator keyStyleCloseBegin =
                    closeTags.begin() + NUMBER_BUILTIN_STATES;
                openTags.erase ( keyStyleOpenBegin, openTags.end() );
                closeTags.erase ( keyStyleCloseBegin, closeTags.end() );
            }
            // add new keyword tag delimiters
            for ( unsigned int i=0; i< currentSyntax->getKeywordClasses().size(); i++ ) {
                openTags.push_back ( getKeywordOpenTag ( i ) );
                closeTags.push_back ( getKeywordCloseTag ( i ) );
            }
            
            //test balloon
            string overrideSpacer(currentSyntax->getOverrideConfigVal("spacer"));
            if (!overrideSpacer.empty()) {
                spacer = overrideSpacer;
            }
            string overrideMaskWS(currentSyntax->getOverrideConfigVal("maskws"));
            if (!overrideMaskWS.empty()) {
                maskWs = overrideMaskWS=="true";
            }
            
        }
    }
    return result;
}

bool CodeGenerator::validateInputStream()
{
    if ( !in ) return false;

    // it is not possible to move stream pointer back with stdin
    if ( ( int ) in->tellg() == -1 ) // -1 : stdin
        return true;

    // Sources: http://en.wikipedia.org/wiki/Magic_number_(programming)
    // Magic configuration of "file"
    // This is intended for web plugins - only check filetypes often found in the net
    char magic_gif[]    = {'G','I','F','8', 0};
    char magic_png[]    = {'\x89','P','N','G', 0};
    char magic_java[]   = {'\xCA','\xFE','\xBA','\xBE', 0};
    char magic_jpeg[]   = {'\xFF','\xD8','\xFF', 0};
    char magic_bmp[]    = {'B','M', 0};
    char magic_pdf[]    = {'%','P','D','F', 0};
    char magic_utf8[]   = {'\xEF','\xBB','\xBF',0};
    char magic_rar[]    = {'R','a','r','!', 0};
    char magic_zip[]    = {'P','K','\x03','\x04', 0};
    char magic_ace[]    = {'*','*','A','C','E','*','*', 0};
    char magic_tgz[]    = {'\x8b','\x1f', '\x00', '\x08', 0};
    char magic_bzip[]   = {'B','Z', 0};

    char* magic_table[] = {magic_utf8,
                           magic_gif, magic_png, magic_jpeg, magic_bmp, magic_pdf,
                           magic_java,
                           magic_rar, magic_zip, magic_ace, magic_tgz, magic_bzip,
                           0
                          };

    char buffer [10]= {0};
    in->read ( buffer,8 );  //only read the first 8 bytes of input stream

    int magic_index=0;
    while ( magic_table[magic_index] ) {
        if ( !strncmp ( buffer, magic_table[magic_index], strlen ( magic_table[magic_index] ) ) ) {
            break;
        }
        magic_index++;
    }
    int streamReadPos=0;
    if ( magic_table[magic_index] == magic_utf8 ) {
        //setEncoding("utf-8");
        streamReadPos=3; // remove UTF-8 magic number from output
    }

    in -> seekg ( streamReadPos, ios::beg );
    in-> clear();  // clear fail bit to continue reading

    return !magic_table[magic_index] // points to 0 if no pattern was found
           || magic_table[magic_index] == magic_utf8;
}

void CodeGenerator::applyPluginChunk(const string& fctName, string *result, bool *keepDefault) {
    
    if ( pluginChunks.size()) {
    
        Diluculum::LuaState luaState;

        Diluculum::LuaValueList chunkParams;
        chunkParams.push_back(currentSyntax->getDescription());
        for (unsigned int i=0; i<pluginChunks.size(); i++) {
            luaState.call(*pluginChunks[i], chunkParams, "format user function");
        }
        
        if (luaState.globals().count(fctName)) {
            Diluculum::LuaFunction* documentFct=new Diluculum::LuaFunction(luaState[fctName].value().asFunction());
        
            luaState["HL_INPUT_FILE"] = luaState["HL_PLUGIN_PARAM"] = pluginParameter;
            luaState["HL_OUTPUT"] = outputType;
            luaState["HL_FORMAT_HTML"]=HTML;
            luaState["HL_FORMAT_XHTML"]=XHTML;
            luaState["HL_FORMAT_TEX"]=TEX;
            luaState["HL_FORMAT_LATEX"]=LATEX;
            luaState["HL_FORMAT_RTF"]=RTF;
            luaState["HL_FORMAT_ANSI"]=ESC_ANSI;
            luaState["HL_FORMAT_XTERM256"]=ESC_XTERM256;
            luaState["HL_FORMAT_TRUECOLOR"]=ESC_TRUECOLOR;
            luaState["HL_FORMAT_SVG"]=SVG;
            luaState["HL_FORMAT_BBCODE"]=BBCODE;
            luaState["HL_FORMAT_PANGO"]=PANGO;
            luaState["HL_FORMAT_ODT"]=ODTFLAT;
            
            Diluculum::LuaValueList params;
            Diluculum::LuaValueMap options;
            options[Diluculum::LuaValue("title")] =  Diluculum::LuaValue( docTitle );   
            options[Diluculum::LuaValue("encoding")] =  Diluculum::LuaValue(encoding);   
            options[Diluculum::LuaValue("fragment")] =  Diluculum::LuaValue(fragmentOutput);   
            options[Diluculum::LuaValue("font")] =  Diluculum::LuaValue(getBaseFont());   
            options[Diluculum::LuaValue("fontsize")] =  Diluculum::LuaValue(getBaseFontSize());   

            params.push_back(inputFilesCnt);
            params.push_back(processedFilesCnt);
            params.push_back(options);
            
            Diluculum::LuaValueList res=luaState.call ( *documentFct, params, fctName+" call")  ;
            if (res.size()>=1) {
                *keepDefault=false;
                *result = res[0].asString();
                if (res.size()==2)
                    *keepDefault = res[1].asBoolean();
            }
            delete documentFct;
        }
    }
}

void CodeGenerator::printHeader()
{
    bool keepDefaultHeader=true;
    string pluginHeader;
    
    processedFilesCnt++;
    
    applyPluginChunk("DocumentHeader", &pluginHeader, &keepDefaultHeader);

    if ( ! fragmentOutput && keepDefaultHeader)
        *out << getHeader();
    
    *out << pluginHeader; 
   
    if ( !fragmentOutput || keepInjections)
        *out << currentSyntax->getHeaderInjection();
}

void CodeGenerator::printFooter()
{
    
    bool keepDefaultFooter=true;
    string pluginFooter;
    
    applyPluginChunk("DocumentFooter", &pluginFooter, &keepDefaultFooter);
    
    if ( !fragmentOutput || keepInjections)
        *out << currentSyntax->getFooterInjection();

    *out << pluginFooter; 
    
    if ( ! fragmentOutput && keepDefaultFooter )
        *out << getFooter();
}

ParseError CodeGenerator::generateFile ( const string &inFileName,
        const string &outFileName )
{
    if ( !docStyle.found() ) {
        return BAD_STYLE;
    }

    reset();

    ParseError error=PARSE_OK;

    inFile=inFileName;
    outFile=outFileName;
        
    in = ( inFileName.empty() ? &cin :new ifstream ( inFileName.c_str() ) );

    if ( validateInput )
        if ( !validateInputStream() ) error= BAD_INPUT;

    if ( !in->fail() && error==PARSE_OK ) {
        out = ( outFileName.empty() ? &cout :new ofstream ( outFileName.c_str() ) );
        if ( out->fail() ) {
            error=BAD_OUTPUT;
        }
    }

    if ( in->fail() ) {
        error=BAD_INPUT;
    }

    if ( error==PARSE_OK ) {
        if ( formatter != NULL ) {
            formatter->init ( new astyle::ASStreamIterator ( in ) );
        }
        printHeader();
        printBody();
        printFooter();
    }

    if ( !outFileName.empty() ) {
        delete out;
        out=NULL;
    }
    if ( !inFileName.empty() ) {
        delete in;
        in=NULL;
    }
    return error;
}

string CodeGenerator::generateString ( const string &input )
{

    if ( !docStyle.found() ) {
        return "";
    }

    reset();

    in = new istringstream ( input );
    out = new ostringstream ();

    if ( in->fail() || out->fail() ) {
        return "";
    }

    if ( formatter != NULL ) {
        formatter->init ( new astyle::ASStreamIterator ( in ) );
    }
    printHeader();
    printBody();
    printFooter();

    string result = static_cast<ostringstream*> ( out )->str();

    delete out;
    out=NULL;
    delete in;
    in=NULL;

    return result;
}

string CodeGenerator::generateStringFromFile ( const string &inFileName )
{

    if ( !docStyle.found() ) {
        return "";
    }

    reset();

    inFile = inFileName;
    
    in = new ifstream ( inFileName.c_str() );
    out = new ostringstream ();

    if ( in->fail() || out->fail() ) {
        return "";
    }

    if ( validateInput && !validateInputStream() ) {
        return "ERROR: detected binary input";
    }

    if ( formatter != NULL ) {
        formatter->init ( new astyle::ASStreamIterator ( in ) );
    }
    printHeader();
    printBody();
    printFooter();

    string result = static_cast<ostringstream*> ( out )->str();

    delete out;
    out=NULL;
    delete in;
    in=NULL;

    return result;
}

unsigned int CodeGenerator::getStyleID ( State s, unsigned int kwClassID )
{
    if ( s==KEYWORD && kwClassID ) {
        return NUMBER_BUILTIN_STATES + kwClassID-1;
    }
    return ( unsigned int ) s ;
}

void CodeGenerator::openTag ( State s )
{
    *out << openTags[ ( unsigned int ) s];
    currentState=s;

}

void CodeGenerator::closeTag ( State s )
{
    *out << closeTags[ ( unsigned int ) s];
    flushWs(2);
    currentState=_UNKNOWN;
}

void CodeGenerator::openKWTag ( unsigned int kwClassID )
{
    *out << openTags.at(getStyleID ( KEYWORD, kwClassID ) );
    currentState=KEYWORD;
}

void CodeGenerator::closeKWTag ( unsigned int kwClassID )
{
    *out << closeTags.at(getStyleID ( KEYWORD, kwClassID ) );
    flushWs(3);
    currentState=_UNKNOWN;
}

bool CodeGenerator::loadEmbeddedLang(const string&embedLangDefPath)
{
    if (nestedLangs.empty()) {
        nestedLangs.push(currentSyntax->getCurrentPath() );
    }
    if (nestedLangs.top() != embedLangDefPath) {
        nestedLangs.push(embedLangDefPath);
    }
    LoadResult res = loadLanguage(embedLangDefPath, true);
    //pass end delimiter regex to syntax description
    currentSyntax->restoreLangEndDelim(embedLangDefPath);
    return res == LOAD_OK;
}

///////////////////////////////////////////////////////////////////////////////

void CodeGenerator::processRootState()
{
    bool eof=false,
         firstLine=true; // avoid newline before printing the first output line

    applySyntaxTestCase = inFile.find("syntax_test_")!=string::npos;
    
    if ( currentSyntax->highlightingDisabled() ) {
        string line;
        while ( getline ( *in, line ) && lineNumber < maxLineCnt ) {
            ++lineNumber;
            insertLineNumber ( !firstLine );
            flushWs(4);
            firstLine=false;
            if (lineNumber>=startLineCntCurFile && lineNumber <=maxLineCnt)
                maskString ( *out, line );
        }
        *out << flush;
        return;
    }

    if (!embedLangStart.empty()) {
        if (!loadEmbeddedLang(currentSyntax->getNewPath(embedLangStart))) return;
    }

    State state=STANDARD;

    openTag ( STANDARD );
    do {
        // determine next state
        state= getCurrentState(STANDARD);

        // handle current state
        switch ( state ) {
        case KEYWORD:
            closeTag ( STANDARD );
            eof=processKeywordState ( state );
            openTag ( STANDARD );
            break;
        case NUMBER:
            closeTag ( STANDARD );
            eof=processNumberState();
            openTag ( STANDARD );
            break;
        case ML_COMMENT:
            closeTag ( STANDARD );
            eof=processMultiLineCommentState();
            openTag ( STANDARD );
            break;
        case SL_COMMENT:
            closeTag ( STANDARD );
            eof=processSingleLineCommentState();
            openTag ( STANDARD );
            break;
        case STRING:
            closeTag ( STANDARD );
            eof=processStringState ( STANDARD );
            openTag ( STANDARD );
            break;
        case DIRECTIVE:
            closeTag ( STANDARD );
            eof=processDirectiveState();
            openTag ( STANDARD );
            break;
        case ESC_CHAR:
            closeTag ( STANDARD );
            eof=processEscapeCharState();
            openTag ( STANDARD );
            break;
        case SYMBOL:
            closeTag ( STANDARD );
            eof=processSymbolState();
            openTag ( STANDARD );
            break;

        case EMBEDDED_CODE_END:
            closeTag ( STANDARD );
            eof=processSyntaxChangeState(state);
            openTag ( STANDARD );
            break;
        case _EOL:

            // XTERM256 fix (issue with less cmd)
            if  (!firstLine || showLineNumbers) {
                closeTag ( STANDARD );
            }
            insertLineNumber ( !firstLine );
            if (!firstLine || showLineNumbers) {
                flushWs(5);
                stateTraceCurrent.clear();
                openTag ( STANDARD );
            }
            firstLine=false;
            break;
        case _EOF:
            eof=true;
            break;
        case _WS:
            processWsState();
            break;
        default:
            printMaskedToken ();
            break;
        }
    } while ( !eof );
    closeTag ( STANDARD );

    if (currentSyntax->getDecorateLineEndFct()) {
        Diluculum::LuaValueList res=callDecorateLineFct(false);
        if (res.size()==1) {
            *out << res[0].asString();
        }
    }

    printNewLines = !noTrailingNewLine;
    *out << getNewLine();
    *out << flush;
}

bool CodeGenerator::processSyntaxChangeState(State myState)
{
    State newState=STANDARD;
    bool eof=false,
         exitState=false;

    openTag ( KEYWORD );
    do {

        if (myState==EMBEDDED_CODE_END) {
            if (!nestedLangs.empty()) {
                nestedLangs.pop();
            }
            // load host language syntax
            if (!nestedLangs.empty()) {
                loadLanguage(nestedLangs.top(), true);
            }
            matchRegex(line, EMBEDDED_CODE_BEGIN); // match remaining line using the host syntax
        }
        
        printMaskedToken ( newState!=_WS );

        newState= getCurrentState(myState);

        switch ( newState ) {
        case _WS:
            processWsState();
            break;
        case _EOL:
            insertLineNumber();
            exitState=true;
            break;
        case _EOF:
            eof = true;
            break;
        default:
            exitState=true;
            break;
        }
    } while (  !exitState  &&  !eof );
    closeTag ( KEYWORD );

    return eof;
}


bool CodeGenerator::processKeywordState ( State myState )
{
    State newState=STANDARD;
    unsigned int myClassID=currentKeywordClass;
    bool eof=false,
         exitState=false;

    openKWTag ( myClassID );
    do {
        printMaskedToken ( newState!=_WS,
                           ( currentSyntax->isIgnoreCase() ) ? keywordCase : StringTools::CASE_UNCHANGED );
        newState= getCurrentState(myState);
        switch ( newState ) {
        case _WS:
            processWsState();
            exitState=isolateTags;
            break;
        case _EOL:
            insertLineNumber();
            exitState=true;
            
            break;
        case _EOF:
            eof = true;
            break;
        case KEYWORD_END:
            exitState=true;
            break;
        default:
            exitState= ( myClassID!=currentKeywordClass ) || ( myState!=newState );
            break;
        }
    } while ( !exitState  &&  !eof );

    closeKWTag ( myClassID );

    currentKeywordClass=0;
    return eof;
}

bool CodeGenerator::processNumberState()
{
    State newState=STANDARD;
    bool eof=false,
         exitState=false;
    openTag ( NUMBER );
    do {
        printMaskedToken ( newState!=_WS );
        newState= getCurrentState(NUMBER);
        switch ( newState ) {
        case _WS:
            processWsState();
            exitState=isolateTags;
            break;
        case _EOL:
            insertLineNumber();
            exitState=true;
            break;
        case _EOF:
            eof = true;
            break;
        default:
            exitState=newState!=NUMBER;
            break;
        }
    } while ( !exitState && !eof );

    closeTag ( NUMBER );
    return eof;
}


bool CodeGenerator::processMultiLineCommentState()
{
    int commentCount=1;
    int openDelimID=currentSyntax->getOpenDelimiterID ( token, ML_COMMENT);
    State newState=STANDARD;
    bool eof=false, exitState=false, containedTestCase=false;
    unsigned int startColumn=lineIndex - token.size() ;
    openTag ( ML_COMMENT );
    do {
        printMaskedToken (newState!=_WS );
        newState= getCurrentState(ML_COMMENT);

        switch ( newState ) {
        case _WS:
            processWsState();
            break;
        case _EOL:
            wsBuffer += closeTags[ML_COMMENT];
            insertLineNumber();
            wsBuffer += openTags[ML_COMMENT];
            startColumn=0;
            break;
        case _EOF:
            eof = true;
            break;
        case _TESTPOS:
            runSyntaxTestcases(token=="<" ? startColumn : lineIndex - 1 );
            printMaskedToken();
            containedTestCase=true;
            break;
        case ML_COMMENT:

            if ( currentSyntax->allowNestedMLComments() ) {
                ++commentCount;
            }
            // if delimiters are equal, close the comment by continueing to
            // ML_COMMENT_END section
            if (currentSyntax->delimiterIsDistinct(currentSyntax->getOpenDelimiterID ( token, ML_COMMENT  ))) break;

        case ML_COMMENT_END:

            if (!currentSyntax->matchesOpenDelimiter (token,  ML_COMMENT_END, openDelimID)) {
                break;
            }
            commentCount--;
            if ( !commentCount ) {
                printMaskedToken();
                exitState=true;
            }
            break;
        default:
            break;
        }
    } while ( !exitState  &&  !eof );

    closeTag ( ML_COMMENT );
   
    if (containedTestCase){
        stateTraceCurrent.clear();
    }
    return eof;
}


bool CodeGenerator::processSingleLineCommentState()
{
    State newState=STANDARD;
    bool eof=false, exitState=false, containedTestCase=false;
    unsigned int startColumn = lineIndex - token.size() ;

    openTag ( SL_COMMENT );
    do {
        printMaskedToken ( newState!=_WS );
        newState= getCurrentState(SL_COMMENT);

        switch ( newState ) {
        case _WS:
            processWsState();
            break;
        case _EOL:
            printMaskedToken();
            if ( preFormatter.isEnabled() && preFormatter.isWrappedLine ( lineNumber-1 ) ) {
                exitState=false;
            } else {
                exitState=true;
            }
            if ( !exitState ) wsBuffer += closeTags[SL_COMMENT];
            insertLineNumber();
            if ( !exitState ) wsBuffer += openTags[SL_COMMENT];

            break;
        case _EOF:
            eof = true;
            break;
        case _TESTPOS:
            runSyntaxTestcases(token=="<" ? startColumn : lineIndex - 1 );
            printMaskedToken();
            containedTestCase=true;
            break;
     
        default:
            break;
        }
    } while ( !exitState  &&  !eof );

    closeTag ( SL_COMMENT );
    
    if (containedTestCase) {
        stateTraceCurrent.clear();
    }
    
    return eof;
}

bool CodeGenerator::processDirectiveState()
{
    State  newState=STANDARD;
    bool eof=false, exitState=false;

    openTag ( DIRECTIVE );
    do {
        printMaskedToken ( newState!=_WS );
        newState= getCurrentState(DIRECTIVE);
        switch ( newState ) {
        case _WS:
            processWsState();
            break;
        case DIRECTIVE_END:
            printMaskedToken();
            exitState=true;
            break;
        case _EOL:
            printMaskedToken();
            
            if ( preFormatter.isEnabled() && preFormatter.isWrappedLine ( lineNumber-1 ) ) {
                exitState=false;
            } else {
                if (currentSyntax->getContinuationChar()!=0x13){
                    exitState= ( terminatingChar!=currentSyntax->getContinuationChar() );
                } 
            }
            if ( !exitState ) wsBuffer += closeTags[DIRECTIVE];
            insertLineNumber();
            if ( !exitState ) wsBuffer += openTags[DIRECTIVE];
            break;
        case ML_COMMENT:
            closeTag ( DIRECTIVE );
            eof= processMultiLineCommentState();
            openTag ( DIRECTIVE );
            break;
        case SL_COMMENT:
            closeTag ( DIRECTIVE );
            eof= processSingleLineCommentState();
            openTag ( DIRECTIVE );
            exitState=true;
            break;
        case STRING:
            closeTag ( DIRECTIVE );
            eof=processStringState ( DIRECTIVE );
            openTag ( DIRECTIVE );
            break;
        case _EOF:
            eof = true;
            break;
        default:
            break;
        }
    } while ( !exitState && !eof );

    closeTag ( DIRECTIVE );
    return eof;
}

bool CodeGenerator::processStringState ( State oldState )
{
    State newState=STANDARD;
    bool eof=false, exitState=false;
    bool returnedFromOtherState=false;

    State myState= ( oldState==DIRECTIVE ) ? DIRECTIVE_STRING : STRING;

    int openDelimID=currentSyntax->getOpenDelimiterID ( token, myState);
    string openDelim=token;

    //Raw String by definition:
    bool isRawString=currentSyntax->delimiterIsRawString(openDelimID);

    // Test if character before string open delimiter token equals to the
    // raw string prefix (Example: r" ", r""" """ in Python)

    //Raw String Prefix:
    if ( lineIndex>token.length() &&line[lineIndex-token.length()-1]==currentSyntax->getRawStringPrefix() ) {
        isRawString=true;
    }

    openTag ( myState );
    do {
        // true if last token was an escape char
        if ( !returnedFromOtherState ) {
            printMaskedToken (newState!=_WS );
        }
        returnedFromOtherState=false;
        newState= getCurrentState(myState);

        switch ( newState ) {
        case _WS:
            processWsState();
            break;
        case _EOL:
            wsBuffer += closeTags[myState];
            insertLineNumber();
            wsBuffer += openTags[myState];
            break;
        case STRING_END:
            if (resultOfHook || currentSyntax->matchesOpenDelimiter (token,  STRING_END, openDelimID)) {
                if (currentSyntax->assertDelimEqualLength()) {
                    exitState= openDelim.length()==token.length();
                } else {
                    exitState= true;
                }
                printMaskedToken();
            }
            break;
        case STRING:
            // if there exist multiple string delimiters, close string if
            // current delimiter is equal to the opening delimiter
            exitState=currentSyntax->delimiterIsDistinct(currentSyntax->getOpenDelimiterID ( token, STRING  ))&&token==openDelim;
            printMaskedToken();
            break;
        case ESC_CHAR:
            if ( !isRawString ) {
                closeTag ( myState );
                eof=processEscapeCharState();
                openTag ( myState );
                returnedFromOtherState=true;
            } else {
                // FIXME not a fix for Python r"""\"""
                exitState=token.size()>1 && token[1] == openDelim[0];
                printMaskedToken();
            }
            break;
        case STRING_INTERPOLATION:
            closeTag ( myState );
            eof=processInterpolationState();
            openTag ( myState );
            returnedFromOtherState=true;
            break;
    
        case _EOF:
            eof = true;
            break;
        default:
            printMaskedToken();
            break;
        }
    } while ( !exitState && !eof );

    closeTag ( myState );

    return eof;
}

bool CodeGenerator::processSymbolState()
{

    State newState=STANDARD;
    bool eof=false,
         exitState=false;

    openTag ( SYMBOL );
    do {
        printMaskedToken ( newState!=_WS );
        newState= getCurrentState(SYMBOL);
        switch ( newState ) {
        case _WS:
            processWsState();
            exitState=isolateTags;
            break;
        case _EOL:
            insertLineNumber();
            exitState=true;
            break;
        case _EOF:
            eof = true;
            break;
        default:
            exitState=newState!=SYMBOL;
            break;
        }
    } while ( !exitState && !eof );

    closeTag ( SYMBOL );
    return eof;
}

bool CodeGenerator::processEscapeCharState()
{
    State newState=STANDARD;
    bool eof=false, exitState=false;
    openTag ( ESC_CHAR );
    do {
        printMaskedToken (newState!=_WS );
        newState= getCurrentState(ESC_CHAR);
        switch ( newState ) {
        case _EOL:
            insertLineNumber();
            exitState=true;
            break;
        case _WS:
            processWsState();
            exitState=isolateTags;
            break;
        case _EOF:
            eof = true;
            break;
        default:
            exitState=newState!=ESC_CHAR;
            break;
        }
    } while ( !exitState && !eof );

    closeTag ( ESC_CHAR );
    return eof;
}

bool CodeGenerator::processInterpolationState()
{
    State newState=STANDARD;
    bool eof=false, exitState=false;
    openTag ( STRING_INTERPOLATION );
    do {
        printMaskedToken (newState!=_WS );
        newState= getCurrentState(STRING_INTERPOLATION);
        switch ( newState ) {
        case _EOL:
            insertLineNumber();
            exitState=true;
            break;
        case _WS:
            processWsState();
            exitState=isolateTags;
            break;
        case _EOF:
            eof = true;
            break;
        default:
            exitState=newState!=STRING_INTERPOLATION;
            break;
        }
    } while ( !exitState && !eof );

    closeTag ( STRING_INTERPOLATION );
    return eof;
}

void CodeGenerator::processWsState()
{
    if ( !maskWs ) {
        wsBuffer += token;
        token.clear();
        return;
    }

    flushWs(6);

    int cntWs=0;
    lineIndex--;
    PositionState ps(currentState, 0, true);
            
    while ( line[lineIndex]==' ' || line[lineIndex]=='\t' ) {
        ++cntWs;
        ++lineIndex;
    }
    if ( cntWs>1 ) {

        unsigned int styleID=getStyleID ( currentState, currentKeywordClass );
        if ( excludeWs && styleID!=_UNKNOWN ) {
            *out << closeTags[styleID];
        }
        *out << maskWsBegin ;
        for ( int i=0; i<cntWs; i++ ) {
            *out <<  spacer;
            if (applySyntaxTestCase){
                stateTraceCurrent.push_back(ps);
            }
        }
        *out << maskWsEnd;
        if ( excludeWs && styleID!=_UNKNOWN ) {
            *out << openTags[styleID];
        }
    } else {
    
        *out << spacer; //Bugfix fehlender Space nach Strings
        if (applySyntaxTestCase){
            stateTraceCurrent.push_back(ps);            
        }
    }
    token.clear();
}

void CodeGenerator::flushWs(int arg)
{
     PositionState ps(currentState, 0, true);
     //workaround condition
     for ( size_t i=0; i<wsBuffer.size() && ((arg > 3) || ( (arg<4) && lineIndex>1)) && applySyntaxTestCase ; i++ ) {
        stateTraceCurrent.push_back(ps);
        //std::cerr <<"\nflush >"<<wsBuffer<<"< arg:"<<arg;           
    }
     
     //fix canvas whitespace
     if (outputType==ESC_XTERM256 || outputType==ESC_TRUECOLOR){
        *out<< maskWsBegin;
     }
    
    *out<<wsBuffer;
    wsBuffer.clear();
}

string CodeGenerator::getTestcaseName(State s, unsigned int kwClass) {
    switch (s) {
        
        case STANDARD:
            return STY_NAME_STD;
        case STRING:
            return STY_NAME_STR;
        case NUMBER:
            return STY_NAME_NUM;
        case SL_COMMENT:
            return STY_NAME_SLC;
        case ML_COMMENT:
            return STY_NAME_COM;
        case ESC_CHAR:
            return STY_NAME_ESC;
        case DIRECTIVE:
            return STY_NAME_DIR;
        case DIRECTIVE_STRING:
            return STY_NAME_DST;
        case SYMBOL:
            return STY_NAME_SYM;
        case STRING_INTERPOLATION:
            return STY_NAME_IPL;
        case _WS:
            return "ws";
        case KEYWORD: {
            
            if (!kwClass)
                return "ws";
            
            char kwName[5] = {0};
            snprintf(kwName, sizeof(kwName), "kw%c", ('a'+kwClass-1));
            return string(kwName);
        }
        default:
            return "unknown_test";
    }
}

void CodeGenerator::printTrace(const string &s){
    std::cout<<"\n curr "<<lineNumber<<" "<<s<<": ";
    for (unsigned int i=0; i< stateTraceCurrent.size(); i++) {
        std::cout<<" "<<stateTraceCurrent[i].state;
    }
    std::cout<<"\n test "<<lineNumber<<" "<<s<<": ";
    for (unsigned int i=0; i< stateTraceTest.size(); i++) {
        std::cout<<" "<<stateTraceTest[i].state;
    }
    /*
    for (unsigned int i=0; i< stateTrace.size(); i++) {
        std::cout<<" "<<stateTrace[i];
    }
   */
    std::cout<<"\n";
}

void CodeGenerator::runSyntaxTestcases(unsigned int column){
    
    unsigned int assertGroup=0;
    size_t typeDescPos=line.find_first_not_of("\t ^", lineIndex);
    State assertState=_UNKNOWN;
    
    //printTrace("trace 2");
    
    if (typeDescPos!=string::npos) {
    
        if (line.find(STY_NAME_NUM, typeDescPos)==typeDescPos)
            assertState=NUMBER;
        else if (line.find(STY_NAME_STR, typeDescPos)==typeDescPos)
            assertState=STRING;
        else if (line.find(STY_NAME_ESC, typeDescPos)==typeDescPos)
            assertState=ESC_CHAR;
        else if (line.find(STY_NAME_IPL, typeDescPos)==typeDescPos)
            assertState=STRING_INTERPOLATION;
        else if (line.find(STY_NAME_SYM, typeDescPos)==typeDescPos)
            assertState=SYMBOL;
        else if (line.find(STY_NAME_DIR, typeDescPos)==typeDescPos)
            assertState=DIRECTIVE;
        else if (line.find(STY_NAME_SLC, typeDescPos)==typeDescPos)
            assertState=SL_COMMENT;
        else if (line.find(STY_NAME_COM, typeDescPos)==typeDescPos)
            assertState=ML_COMMENT;
        else if (line.find("ws", typeDescPos)==typeDescPos)
            assertState=_WS;
        else if (line.find(STY_NAME_STD, typeDescPos)==typeDescPos)
            assertState=STANDARD;
        else if (line.find(STY_NAME_DST, typeDescPos)==typeDescPos)
            assertState=DIRECTIVE_STRING;
        
        else if (line.find("kw", typeDescPos)==typeDescPos) {
            assertState=KEYWORD;
            if (isalpha(line[typeDescPos+2]))
                assertGroup=line[typeDescPos+2] - 'a' +1;
        }
    
        if (   (assertState!=_WS && stateTraceTest[column].state != assertState && !stateTraceTest[column].isWhiteSpace )
            || (assertState==_WS && !stateTraceTest[column].isWhiteSpace)
            || assertGroup != stateTraceTest[column].kwClass) {
            ostringstream err;
            err << inFile << " line " << lineNumber << ", column "<< column 
                << ": got " << getTestcaseName(stateTraceTest[column].state, stateTraceTest[column].kwClass)  
                << " instead of " << getTestcaseName(assertState, assertGroup);
            failedPosTests.push_back(err.str());
        }
        
    }
    
    lineContainedTestCase=true; 
}


string CodeGenerator::getNewLine()
{
    return (printNewLines) ? newLineTag : "";
}

Diluculum::LuaValueList CodeGenerator::callDecorateLineFct(bool isLineStart)
{

    Diluculum::LuaValueList params;
    params.push_back(Diluculum::LuaValue(lineNumber));

    return currentSyntax->getLuaState()->call ( isLineStart ?
            *currentSyntax->getDecorateLineBeginFct(): *currentSyntax->getDecorateLineEndFct(),
            params,"getDecorateLineFct call")  ;

}

void CodeGenerator::insertLineNumber ( bool insertNewLine )
{
    if ( insertNewLine ) {
        if (currentSyntax->getDecorateLineEndFct()) {
            Diluculum::LuaValueList res=callDecorateLineFct(false);
            if (res.size()==1) {
                wsBuffer +=res[0].asString();
            }
        }

        wsBuffer += getNewLine();
    }

    if (currentSyntax->getDecorateLineBeginFct()) {
        Diluculum::LuaValueList res=callDecorateLineFct(true);
        if (res.size()==1) {
            wsBuffer +=res[0].asString();
        }
    }

    if ( showLineNumbers ) {
        ostringstream os;
        ostringstream numberPrefix;

        os << setw ( getLineNumberWidth() ) << right;
        if( numberCurrentLine ) {
            if ( lineNumberFillZeroes ) {
                os.fill ( '0' );
            }
            os << lineNumber+lineNumberOffset;
        } else {
            os << "";
        }

        numberPrefix << openTags[LINENUMBER];
        maskString ( numberPrefix, os.str() );
        numberPrefix << spacer << closeTags[LINENUMBER];

        wsBuffer += numberPrefix.str();
    }
}

unsigned int CodeGenerator::getLineIndex()
{
    return lineIndex;
}
unsigned int CodeGenerator::getLastLineLength()
{
    return lastLineLength;
}

bool CodeGenerator::printExternalStyle ( const string &outFile )
{
    if ( !includeStyleDef ) {
        ostream *cssOutFile = ( outFile.empty() ? &cout :new ofstream ( outFile.c_str() ) );
        if ( !cssOutFile->fail() ) {
            if (!omitVersionComment) {
                *cssOutFile << styleCommentOpen
                            <<" Style definition file generated by highlight "
                            << HIGHLIGHT_VERSION << ", " << HIGHLIGHT_URL
                            << " " << styleCommentClose << "\n";
            }
            *cssOutFile << getStyleDefinition()
                        << "\n";
            *cssOutFile << readUserStyleDef();
            if ( !outFile.empty() ) delete cssOutFile;
        } else {
            return false;
        }
    }
    return true;
}

string CodeGenerator::readUserStyleDef()
{
    ostringstream ostr;
    if ( !styleInputPath.empty() ) {
        ifstream userStyleDef ( styleInputPath.c_str() );
        if ( userStyleDef ) {
            ostr 	<< "\n" << styleCommentOpen
                    << " Content of " << styleInputPath
                    << ": " <<styleCommentClose << "\n";
            string line;
            while ( getline ( userStyleDef, line ) ) {
                ostr << line << "\n";
            }
            userStyleDef.close();
        } else {
            ostr 	<< styleCommentOpen
                    << " ERROR: Could not include " << styleInputPath
                    << "." << styleCommentClose << "\n";
        }
    }

    string injections=docStyle.getInjections();
    if (!injections.empty()) {
        ostr 	<< "\n" << styleCommentOpen
                << " Plug-in theme injections: " <<styleCommentClose << "\n";
        ostr << injections<<"\n";
    }
    return ostr.str();
}

bool CodeGenerator::initPluginScript(const string& script)
{

    if (script.empty()) return true;

    try {

        userScriptError="";
        Diluculum::LuaState ls;
        
        ls.doFile (script);
        int listIdx=1;

        while (ls["Plugins"][listIdx].value() !=Diluculum::Nil) {

            // Theme plugins
            if (ls["Plugins"][listIdx]["Type"].value().asString()=="theme") {
                if (ls["Plugins"][listIdx]["Chunk"].value().type()==LUA_TFUNCTION) {
                    docStyle.addUserChunk(ls["Plugins"][listIdx]["Chunk"].value().asFunction());
                }
            }
            // Syntax plugins
            else if (ls["Plugins"][listIdx]["Type"].value().asString()=="lang") {
                if (ls["Plugins"][listIdx]["Chunk"].value().type()==LUA_TFUNCTION) {
                    currentSyntax->addUserChunk(ls["Plugins"][listIdx]["Chunk"].value().asFunction());
                }
            }
            // Format plugins
            else if (ls["Plugins"][listIdx]["Type"].value().asString()=="format") {
                if (ls["Plugins"][listIdx]["Chunk"].value().type()==LUA_TFUNCTION) {
                    addUserChunk(ls["Plugins"][listIdx]["Chunk"].value().asFunction());
                }
            }
            
            listIdx++;
        }
    }  catch (Diluculum::LuaError &err) {
        userScriptError=err.what();
        return false;
    }
    return true;
}

}
