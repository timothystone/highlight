/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Die Apr 23 22:16:35 CEST 2002
    copyright            : (C) 2002-2018 by Andre Simon
    email                : a.simon@mailbox.org

   Highlight is a universal source code to HTML converter. Syntax highlighting
   is formatted by Cascading Style Sheets. It's possible to easily enhance
   highlight's parsing database.

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

#include <memory>
#include <algorithm>
#include <Diluculum/LuaState.hpp>

#include "main.h"
#include "../include/datadir.h"
#include "syntaxreader.h"

#define MAX_LINE__WIDTH       80

using namespace std;

void HLCmdLineApp::printVersionInfo()
{
    cout << "\n highlight version "
         << HIGHLIGHT_VERSION
         << "\n Copyright (C) 2002-2019 Andre Simon <a dot simon at mailbox.org>"
         << "\n\n Argparser class"
         << "\n Copyright (C) 2006-2008 Antonio Diaz Diaz <ant_diaz at teleline.es>"
         << "\n\n Artistic Style Classes (3.1)"
         << "\n Copyright (C) 2006-2018 by Jim Pattee <jimp03 at email.com>"
         << "\n Copyright (C) 1998-2002 by Tal Davidson"
         << "\n\n Diluculum Lua wrapper (1.0)"
         << "\n Copyright (C) 2005-2013 by Leandro Motta Barros"
         << "\n\n xterm 256 color matching functions"
         << "\n Copyright (C) 2006 Wolfgang Frisch <wf at frexx.de>"

         << "\n\n This software is released under the terms of the GNU General "
         << "Public License."
         << "\n For more information about these matters, see the file named "
         << "COPYING.\n\n";
}

void HLCmdLineApp::printBadInstallationInfo()
{
    cerr << "highlight: Data directory not found ("<<DataDir::LSB_DATA_DIR<<")."
         " Bad installation or wrong "<< OPT_DATADIR << " parameter."
         << "\n\nCopy the highlight files into one of the directories listed "
         << "in INSTALL.\nYou may also set the data directory with "
         << OPT_DATADIR << ".\n";
}

int HLCmdLineApp::printInstalledFiles(const string& where, const string& wildcard, const string& kind, const string& option, const string& categoryFilterList)
{
    vector <string> filePaths;
    string searchDir = where + wildcard;

    bool directoryOK = Platform::getDirectoryEntries ( filePaths, searchDir, true );
    if ( !directoryOK ) {
        cerr << "highlight: Could not access directory "
             <<  searchDir
             << ", aborted.\n";
        return EXIT_FAILURE;
    }

    sort ( filePaths.begin(), filePaths.end() );
    string suffix, desc;
    Diluculum::LuaValueMap categoryMap;
    cout << "\nInstalled " << kind << "s";
    
    if (categoryFilterList.size())
        cout << " matching \""<<categoryFilterList<<"\"";
    
    cout << " (located in " << where << "):\n\n";
    int matchedFileCnt=0;
    std::set<string> categoryNames;
    std::set<string> categoryFilters;

    istringstream valueStream;
    string catFilter;
    valueStream.str ( StringTools::change_case ( categoryFilterList,StringTools::CASE_LOWER ) );
    while ( getline ( valueStream, catFilter, ';' ) ) {
        categoryFilters.insert ( catFilter );
    }
            
    for ( unsigned int i=0; i< filePaths.size(); i++ ) {
        try {
            Diluculum::LuaState ls;
            highlight::SyntaxReader::initLuaState(ls, filePaths[i],"");
            ls.doFile(filePaths[i]);
            desc = ls["Description"].value().asString();
            
            suffix = ( filePaths[i] ).substr ( where.length() ) ;
            suffix = suffix.substr ( 1, suffix.length()- wildcard.length() );

            unsigned int filterOKCnt=categoryFilters.size();
            if (ls["Categories"].value() !=Diluculum::Nil){
                filterOKCnt=0;

                categoryMap = ls["Categories"].value().asTable();
                
                //TODO: negation
                for(Diluculum::LuaValueMap::const_iterator it = categoryMap.begin(); it != categoryMap.end(); ++it)
                {
                    categoryNames.insert(it->second.asString());
                    if (categoryFilters.size() && categoryFilters.count(it->second.asString())) {
                        ++filterOKCnt;
                    }
                }     
            }
           
            if (filterOKCnt!=categoryFilters.size() && categoryFilters.size() ) continue;
            
            matchedFileCnt++;
            if (kind=="langDef") {
                cout << setw ( 30 ) <<setiosflags ( ios::left ) <<desc<<": "<<suffix;
            
                int extCnt=0;
                for (StringMap::iterator it=dataDir.assocByExtension.begin(); it!=dataDir.assocByExtension.end(); it++) {
                    if (it->second==suffix ) {
                        cout << ((++extCnt==1)?" ( ":" ")<<it->first;
                    }
                }
                cout << ((extCnt)?" )":"");
            } else {
                cout << setw ( 30 ) <<setiosflags ( ios::left ) <<suffix<<": "<<desc;
            
            }
            cout << endl;
        } catch (std::runtime_error &error) {
            cout << "Failed to read '" << filePaths[i] << "': " << error.what() << endl;
        }
    }
    
    if (!matchedFileCnt) {
        cout <<"No files found." << endl;
    } else {
        
        if (!categoryFilters.size()){
            cout << "\nFound "<<kind<<" categories:\n\n";
            for (std::set<string>::iterator it=categoryNames.begin(); it!=categoryNames.end(); ++it)
                std::cout << *it<< ' ';
            cout << "\n";
        }
        
        cout <<"\nUse name of the desired "<<kind
            << " with --" << option << ". Filter categories with --list-cat." << endl;
            
        if (kind=="theme") {
            cout <<"\nApply --base16 to select a Base16 theme." << endl;
        }
        
        printConfigInfo();
    }
    
    return EXIT_SUCCESS;
}

void HLCmdLineApp::printDebugInfo ( const highlight::SyntaxReader *lang,
                                    const string & langDefPath )
{
    if (!lang) return;
    
    map <int, string> HLStateMap;
    
    cerr << "\nLoading language definition:\n" << langDefPath;
    cerr << "\n\nDescription: " << lang->getDescription();
    Diluculum::LuaState* luaState=lang->getLuaState();
    if (luaState) {
        cerr << "\n\nLUA GLOBALS:\n" ;
        Diluculum::LuaValueMap::iterator it;
        Diluculum::LuaValueMap glob =luaState->globals();
        string elemName;
        for(it = glob.begin(); it != glob.end(); it++) {
            Diluculum::LuaValue first = it->first;
            Diluculum::LuaValue second = it->second;
            elemName = first.asString();
            std::cerr << elemName<<": ";
            switch (second.type()) {
            case  LUA_TSTRING:
                cerr << "string [ "<<second.asString()<<" ]";
                break;
            case  LUA_TNUMBER:
                cerr << "number [ "<<second.asNumber()<<" ]";
                if (elemName.find("HL_")==0 && elemName.find("HL_FORMAT")==string::npos)
                    HLStateMap[second.asNumber()] = elemName;
                break;
            case  LUA_TBOOLEAN:
                cerr << "boolean [ "<<second.asBoolean()<<" ]";
                break;
            default:
                cerr << second.typeName();
            }
            cerr << endl;
        }

    }
    
    highlight::RegexElement *re=NULL;
    for ( unsigned int i=0; i<lang->getRegexElements().size(); i++ )
    {
        if (i==0)
             cerr << "\nREGEX:\n";
   
        re = lang->getRegexElements() [i];
        cerr << "State "<<re->open<< " ("<< HLStateMap[re->open]<<  "):\t"<<re->pattern <<"\n";
    }
    
    highlight::KeywordMap::iterator it;
    highlight::KeywordMap keys=lang->getKeywords();
    for ( it=keys.begin(); it!=keys.end(); it++ ) {
        if (it==keys.begin())
            cerr << "\nKEYWORDS:\n";
    
        cerr << " "<< it->first << "("<< it->second << ")";
    }
    cerr <<"\n\n";
}

void HLCmdLineApp::printConfigInfo ( )
{
    cout << "\nConfig file search directories:\n";
    dataDir.printConfigPaths();
    cout << "\nFiletype config file:\n"<<dataDir.getFiletypesConfPath ( "filetypes" ) <<"\n";
    cout << endl;
#ifdef HL_DATA_DIR
    cout << "Compiler directive HL_DATA_DIR = " <<HL_DATA_DIR<< "\n";
#endif
#ifdef HL_CONFIG_DIR
    cout << "Compiler directive HL_CONFIG_DIR = " <<HL_CONFIG_DIR<< "\n";
#endif
    cout << endl;
}

int HLCmdLineApp::getNumDigits ( int i )
{
    int res=0;
    while ( i ) {
        i/=10;
        ++res;
    }
    return res;
}

void HLCmdLineApp::printProgressBar ( int total, int count )
{
    if ( !total ) return;
    int p=100*count / total;
    int numProgressItems=p/10;
    cout << "\r[";
    for ( int i=0; i<10; i++ ) {
        cout << ( ( i<numProgressItems ) ?"#":" " );
    }
    cout<< "] " <<setw ( 3 ) <<p<<"%, "<<count << " / " << total << "  " <<flush;
    if ( p==100 ) {
        cout << endl;
    }
}

void HLCmdLineApp::printCurrentAction ( const string&outfilePath,
                                        int total, int count, int countWidth )
{
    cout << "Writing file "
         << setw ( countWidth ) << count
         << " of "
         << total
         << ": "
         << outfilePath
         << "\n";
}

void HLCmdLineApp::printIOErrorReport ( unsigned int numberErrorFiles,
                                        vector<string> & fileList,
                                        const string &action )
{
    cerr << "highlight: Could not "
         << action
         << " file"
         << ( ( numberErrorFiles>1 ) ?"s":"" ) <<":\n";
    copy ( fileList.begin(), fileList.end(), ostream_iterator<string> ( cerr, "\n" ) );
    if ( fileList.size() < numberErrorFiles ) {
        cerr << "... ["
             << ( numberErrorFiles - fileList.size() )
             << " of "
             << numberErrorFiles
             << " failures not shown, use --"
             << OPT_VERBOSE
             << " switch to print all failures]\n";
    }
}

vector <string> HLCmdLineApp::collectPluginPaths(const vector<string>& plugins)
{
    vector<string> absolutePaths;
    for (unsigned int i=0; i<plugins.size(); i++) {
        if (Platform::fileExists(plugins[i])) {
            absolutePaths.push_back(plugins[i]);
        } else {
            absolutePaths.push_back(dataDir.getPluginPath(plugins[i]+".lua"));
        }
    }
    return absolutePaths;
}

int HLCmdLineApp::run ( const int argc, const char*argv[] )
{
    CmdLineOptions options ( argc, argv );

    // set data directory path, where /langDefs and /themes reside
    string dataDirPath = ( options.getDataDir().empty() ) ?  Platform::getAppPath() :options.getDataDir();

    if ( options.printVersion() ) {
        printVersionInfo();
        return EXIT_SUCCESS;
    }

    dataDir.initSearchDirectories ( dataDirPath );

    if ( options.printHelp() ) {
        Help::printHelp(options.getHelpTopic());
        return EXIT_SUCCESS;
    }

    if ( options.printConfigInfo() ) {
        printConfigInfo ( );
        return EXIT_SUCCESS;
    }

    //call before printInstalledLanguages!
    dataDir.loadFileTypeConfig ( "filetypes" );

    string scriptKind=options.getListScriptKind();
    if (scriptKind.length()) {
        if ( scriptKind.find("theme")==0 ) {
            return printInstalledFiles(dataDir.getThemePath(), "*.theme", "theme", OPT_STYLE, options.getCategories());
        }
        else if ( scriptKind.find("plug")==0 ) {
            return printInstalledFiles(dataDir.getPluginPath(), "*.lua", "plug-in", OPT_PLUGIN, options.getCategories());
        }
        else if (  scriptKind.find("lang")==0 ) {
            return printInstalledFiles(dataDir.getLangPath(), "*.lang", "langDef", OPT_SYNTAX, options.getCategories());
        } else {
            cerr << "highlight: Unknown script type '"<< scriptKind << "'. Apply one of 'themes', 'langs' or 'plug-ins'.\n";
            return EXIT_FAILURE;
        }
    }

    const vector <string> inFileList=options.getInputFileNames();

    if ( options.enableBatchMode() && inFileList[0].empty() ) {
        return EXIT_FAILURE;
    }

    string themePath=options.getAbsThemePath().empty() ? dataDir.getThemePath ( options.getThemeName(), options.useBase16Theme() ): options.getAbsThemePath();

    unique_ptr<highlight::CodeGenerator> generator ( highlight::CodeGenerator::getInstance ( options.getOutputType() ) );

    generator->setHTMLAttachAnchors ( options.attachLineAnchors() );
    generator->setHTMLOrderedList ( options.orderedList() );
    generator->setHTMLInlineCSS ( options.inlineCSS() );
    generator->setHTMLEnclosePreTag ( options.enclosePreTag() );
    generator->setHTMLAnchorPrefix ( options.getAnchorPrefix() );
    generator->setHTMLClassName ( options.getClassName() );

    generator->setLATEXReplaceQuotes ( options.replaceQuotes() );
    generator->setLATEXNoShorthands ( options.disableBabelShorthands() );
    generator->setLATEXPrettySymbols ( options.prettySymbols() );
    generator->setLATEXBeamerMode ( options.enableBeamerMode() );
    
    generator->setRTFPageSize ( options.getPageSize() );
    generator->setRTFCharStyles ( options.includeCharStyles() );
    generator->setRTFPageColor ( options.includePageColor() );
    
    generator->setSVGSize ( options.getSVGWidth(),  options.getSVGHeight() );

    generator->setESCCanvasPadding ( options.getCanvasPadding() );

    if (options.useCRDelimiter())
        generator->setEOLDelimiter('\r');

    generator->setValidateInput ( options.validateInput() );
    generator->setNumberWrappedLines ( options.numberWrappedLines() );

    generator->setStyleInputPath ( options.getStyleInFilename() );
    generator->setStyleOutputPath ( options.getStyleOutFilename() );
    generator->setIncludeStyle ( options.includeStyleDef() );
    generator->setPrintLineNumbers ( options.printLineNumbers(), options.getNumberStart() );
    generator->setPrintZeroes ( options.fillLineNrZeroes() );
    generator->setFragmentCode ( options.fragmentOutput() );
    generator->setOmitVersionComment ( options.omitVersionInfo() );
    generator->setIsolateTags ( options.isolateTags() );

    generator->setKeepInjections ( options.keepInjections());
    generator->setPreformatting ( options.getWrappingStyle(),
                                  ( generator->getPrintLineNumbers() ) ?
                                  options.getLineLength() - options.getNumberWidth() : options.getLineLength(),
                                  options.getNumberSpaces() );

    generator->setEncoding ( options.getEncoding() );
    generator->setBaseFont ( options.getBaseFont() ) ;
    generator->setBaseFontSize ( options.getBaseFontSize() ) ;
    generator->setLineNumberWidth ( options.getNumberWidth() );
    generator->setStartingNestedLang( options.getStartNestedLang());
    generator->disableTrailingNL(options.disableTrailingNL());
    generator->setPluginParameter(options.getPluginParameter());
    
    if (options.getLineRangeStart()>0 && options.getLineRangeEnd()>0){
        generator->setStartingInputLine(options.getLineRangeStart());
        generator->setMaxInputLineCnt(options.getLineRangeEnd());    
    }
    
    bool styleFileWanted = !options.fragmentOutput() || options.styleOutPathDefined();

    const  vector <string> pluginFileList=collectPluginPaths( options.getPluginPaths());
    for (unsigned int i=0; i<pluginFileList.size(); i++) {
        if ( !generator->initPluginScript(pluginFileList[i]) ) {
            cerr << "highlight: "
                 << generator->getPluginScriptError()
                 << " in "
                 << pluginFileList[i]
                 <<"\n";
            return EXIT_FAILURE;
        }
    }

    if ( !generator->initTheme ( themePath ) ) {
        cerr << "highlight: "
             << generator->getThemeInitError()
             << "\n";
        return EXIT_FAILURE;
    }

    if ( options.printOnlyStyle() ) {
        if (!options.formatSupportsExtStyle()) {
            cerr << "highlight: output format supports no external styles.\n";
            return EXIT_FAILURE;
        }
        bool useStdout =  options.getStyleOutFilename() =="stdout" || options.forceStdout();
        string cssOutFile=options.getOutDirectory()  + options.getStyleOutFilename();
        bool success=generator->printExternalStyle ( useStdout?"":cssOutFile );
        if ( !success ) {
            cerr << "highlight: Could not write " << cssOutFile <<".\n";
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    bool formattingEnabled = generator->initIndentationScheme ( options.getIndentScheme() );

    if ( !formattingEnabled && !options.getIndentScheme().empty() ) {
        cerr << "highlight: Undefined indentation scheme "
             << options.getIndentScheme()
             << ".\n";
        return EXIT_FAILURE;
    }
    
    generator->setIndentationOptions(options.getAStyleOptions());

    string outDirectory = options.getOutDirectory();
#ifndef WIN32
    ifstream dirTest ( outDirectory.c_str() );
    if ( !outDirectory.empty() && !options.quietMode() && !dirTest ) {
        cerr << "highlight: Output directory \""
             << outDirectory
             << "\" does not exist.\n";
        return EXIT_FAILURE;
    }
    dirTest.close();
#endif

    bool initError=false, IOError=false;
    unsigned int fileCount=inFileList.size(),
                 fileCountWidth=getNumDigits ( fileCount ),
                 i=0,
                 numBadFormatting=0,
                 numBadInput=0,
                 numBadOutput=0;

    vector<string> badFormattedFiles, badInputFiles, badOutputFiles;
    std::set<string> usedFileNames;
    string inFileName, outFilePath;
    string suffix, lastSuffix;

    if ( options.syntaxGiven() ) { // user defined language definition, valid for all files
        //TODO check for filename given prior to IO redirection
        //--syntax-by-name overrides --syntax
        string syntaxByFile=options.getSyntaxByFilename();
        string testSuffix = syntaxByFile.empty() ? options.getSyntax() : dataDir.getFileSuffix(syntaxByFile); 
        suffix = dataDir.guessFileType (testSuffix, syntaxByFile, syntaxByFile.empty(), true );         
    }
         
    generator->setFilesCnt(fileCount);
        
    while ( i < fileCount && !initError ) {
           
        if ( !options.syntaxGiven() ) { // determine file type for each file
            suffix = dataDir.guessFileType ( dataDir.getFileSuffix ( inFileList[i] ), inFileList[i] );
        }
        if ( suffix.empty()  && options.forceOutput()) suffix="txt"; //avoid segfault
        if ( suffix.empty() ) {
            if ( !options.enableBatchMode() )
                cerr << "highlight: Undefined language definition. Use --"
                     << OPT_SYNTAX << " option.\n";
            if ( !options.forceOutput() ) {
                initError = true;
                break;
            }
        }

        if ( suffix != lastSuffix ) {
        
            string langDefPath=options.getAbsLangPath().empty() ? dataDir.getLangPath ( suffix+".lang" ) : options.getAbsLangPath();

            highlight::LoadResult loadRes= generator-> loadLanguage( langDefPath );

            if ( loadRes==highlight::LOAD_FAILED_REGEX ) {
                cerr << "highlight: Regex error ( "
                     << generator->getSyntaxRegexError()
                     << " ) in "<<suffix<<".lang\n";
                initError = true;
                break;
            } else if ( loadRes==highlight::LOAD_FAILED_LUA ) {
                cerr << "highlight: Lua error ( "
                     << generator->getSyntaxLuaError()
                     << " ) in "<<suffix<<".lang\n";
                initError = true;
                break;
            } else if ( loadRes==highlight::LOAD_FAILED ) {
                // do also ignore error msg if --syntax parameter should be skipped
                if ( ! (options.forceOutput() || options.quietMode() || options.isSkippedExt ( suffix )) ) {
                    cerr << "highlight: Unknown source file extension \""
                         << suffix
                         << "\". Consider the "
                         << (options.enableBatchMode() ? "--skip" : "--force or --syntax")
                         << " option.\n";
                }
                if ( !options.forceOutput() ) {
                    initError = true;
                    break;
                }
            }
            if ( options.printDebugInfo() && loadRes==highlight::LOAD_OK ) {
                printDebugInfo ( generator->getSyntaxReader(), langDefPath );
            }
            lastSuffix = suffix;
        }

        string::size_type pos= ( inFileList[i] ).find_last_of ( Platform::pathSeparator );
        inFileName = inFileList[i].substr ( pos+1 );

        if ( options.enableBatchMode() ) {
            if (usedFileNames.count(inFileName)) {
                string prefix=inFileList[i].substr (2, pos-1 );
                replace (prefix.begin(), prefix.end(), Platform::pathSeparator, '_');
                inFileName.insert(0, prefix);
            } else {
                usedFileNames.insert(inFileName);
            }
            if (!options.forceStdout()){
                outFilePath = outDirectory;
                outFilePath += inFileName;
                outFilePath += options.getOutFileSuffix();
            }
            if ( !options.quietMode() &&  !options.forceStdout() ) {
                if ( options.printProgress() ) {
                    printProgressBar ( fileCount, i+1 );
                } else {
                    printCurrentAction ( outFilePath, fileCount, i+1, fileCountWidth );
                }
            }
        } else if (!options.forceStdout()) {
            outFilePath = options.getSingleOutFilename();
            if ( outFilePath.size() && outFilePath==options.getSingleInFilename() ) {
                cerr << "highlight: Output path equals input path: \""
                     << outFilePath << "\".\n";
                initError = true;
                break;
            }
        }

        if ( options.useFNamesAsAnchors() ) {
            generator->setHTMLAnchorPrefix ( inFileName );
        }

        generator->setTitle ( options.getDocumentTitle().empty() ?
                              inFileList[i]:options.getDocumentTitle() );

        generator->setKeyWordCase ( options.getKeywordCase() );
        highlight::ParseError error = generator->generateFile ( inFileList[i], outFilePath );

        if ( error==highlight::BAD_INPUT ) {
            if ( numBadInput++ < IO_ERROR_REPORT_LENGTH || options.printDebugInfo() ) {
                badInputFiles.push_back ( inFileList[i] );
            }
        } else if ( error==highlight::BAD_OUTPUT ) {
            if ( numBadOutput++ < IO_ERROR_REPORT_LENGTH || options.printDebugInfo() ) {
                badOutputFiles.push_back ( outFilePath );
            }
        }
        if ( formattingEnabled && !generator->formattingIsPossible() ) {
            if ( numBadFormatting++ < IO_ERROR_REPORT_LENGTH || options.printDebugInfo() ) {
                badFormattedFiles.push_back ( outFilePath );
            }
        }
        ++i;
    }

    if ( i  && !options.includeStyleDef()
            && !options.inlineCSS()
            && styleFileWanted
            && options.formatSupportsExtStyle() ) {
        string cssOutFile=outDirectory  + options.getStyleOutFilename();
        bool success=generator->printExternalStyle ( cssOutFile );
        if ( !success ) {
            cerr << "highlight: Could not write " << cssOutFile <<".\n";
            IOError = true;
        }
    }

    if ( i && options.printIndexFile() ) {
        bool success=generator -> printIndexFile ( inFileList, outDirectory );
        if ( !success ) {
            cerr << "highlight: Could not write index file.\n";
            IOError = true;
        }
    }

    if ( numBadInput ) {
        printIOErrorReport ( numBadInput, badInputFiles, "read input" );
        IOError = true;
    }
    if ( numBadOutput ) {
        printIOErrorReport ( numBadOutput, badOutputFiles, "write output" );
        IOError = true;
    }
    if ( numBadFormatting ) {
        printIOErrorReport ( numBadFormatting, badFormattedFiles, "reformat" );
    }
    
    vector<string> posTestErrors = generator->getPosTestErrors();
    if (posTestErrors.size()){
        IOError = true;
        printIOErrorReport ( posTestErrors.size(), posTestErrors, "validate" );
    }
    return ( initError || IOError ) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main ( const int argc, const char *argv[] )
{
    HLCmdLineApp app;
    return app.run ( argc, argv );
}
