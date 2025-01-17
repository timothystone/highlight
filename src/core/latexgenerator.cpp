/***************************************************************************
                          LatexCode.cpp  -  description
                             -------------------
    begin                : Mit Jul 24 2002
    copyright            : (C) 2002 by Andre Simon
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


#include "latexgenerator.h"

namespace highlight
{

LatexGenerator::LatexGenerator()
    : CodeGenerator ( LATEX ),
      replaceQuotes ( false ),
      disableBabelShortHand ( false ),
      prettySymbols ( false ),
      beamerMode ( false )
{
    // avoid "Underfull \hbox (badness 10000)" warnings
    newLineTag = "\\\\\n";
    longLineTag = "\\hspace*{\\fill}" + newLineTag;
    spacer = "\\ ";
    maskWs=true;
    maskWsBegin = "\\hlstd{";
    maskWsEnd = "}";
    excludeWs=true;
    styleCommentOpen="%";
}

LatexGenerator::~LatexGenerator()
{}

void LatexGenerator::printBody()
{
   // if (!beamerMode)
        *out << "\\noindent\n" ;
    if ( ! this->getBaseFont().empty() )
        *out << "\\" << this->getBaseFont() << "\n" ;
    else
        *out << "\\ttfamily\n";
    if ( ! this->getBaseFontSize().empty() && this->getBaseFontSize()!="10")
        *out << "\\" << this->getBaseFontSize() << "\n" ;
    if ( disableBabelShortHand )
        *out << "\\shorthandoff{\"}\n";

    processRootState();

    *out << "\\mbox{}\n"
         << "\\normalfont\n";
    if ( ! this->getBaseFontSize().empty() )
        *out << "\\normalsize\n" ;
    if ( disableBabelShortHand )
        *out << "\\shorthandon{\"}\n";
}

string LatexGenerator::getHeader()
{
    ostringstream os;

    os << (beamerMode ? "\\documentclass{beamer}\n" : "\\documentclass{article}\n")
       << "\\usepackage{color}\n"
       << "\\usepackage{alltt}\n"
       << "\\usepackage[T1]{fontenc}\n";
       
    //if (beamerMode)
    //    os  << "\\usepackage{beamerarticle}\n";
    
    if ( StringTools::change_case ( encoding ) == "utf-8" ) {
        os << "\\usepackage{ucs}\n\\usepackage[utf8x]{inputenc}\n";
    } else if ( encodingDefined() ) {
        os << "\\usepackage[latin1]{inputenc}\n";
    }

    //needed for Righttorque symbol
    if ( preFormatter.isEnabled() ) {
        os << "\\usepackage{marvosym}\n";
    }

    if ( currentSyntax->highlightingEnabled() ) {
        if ( includeStyleDef ) {
            os << "\n"<<getStyleDefinition();
            os << CodeGenerator::readUserStyleDef();
        } else {
            os << "\n\\input {"
               << getStyleOutputPath()
               << "}\n";
        }
    }

    os << "\n\\title{" << docTitle << "}\n"
       << "\\begin{document}\n"
       << "\\pagecolor{bgcolor}\n";
       
    if (beamerMode)
      os  << "\\frame{\\frametitle{Code section}\n";

    if ( prettySymbols ) {
        os<<"\\newsavebox{\\hlboxopenbrace}\n"
          <<"\\newsavebox{\\hlboxclosebrace}\n"
          <<"\\newsavebox{\\hlboxlessthan}\n"
          <<"\\newsavebox{\\hlboxgreaterthan}\n"
          <<"\\newsavebox{\\hlboxdollar}\n"
          <<"\\newsavebox{\\hlboxunderscore}\n"
          <<"\\newsavebox{\\hlboxand}\n"
          <<"\\newsavebox{\\hlboxhash}\n"
          <<"\\newsavebox{\\hlboxat}\n"
          <<"\\newsavebox{\\hlboxbackslash}\n"
          <<"\\newsavebox{\\hlboxpercent}\n"
          <<"\\newsavebox{\\hlboxhat}\n"

          <<"\\setbox\\hlboxopenbrace=\\hbox{\\verb.{.}\n"
          <<"\\setbox\\hlboxclosebrace=\\hbox{\\verb.}.}\n"
          <<"\\setbox\\hlboxlessthan=\\hbox{\\verb.<.}\n"
          <<"\\setbox\\hlboxgreaterthan=\\hbox{\\verb.>.}\n"

          <<"\\setbox\\hlboxdollar=\\hbox{\\verb.$.}\n"
          <<"\\setbox\\hlboxunderscore=\\hbox{\\verb._.}\n"
          <<"\\setbox\\hlboxand=\\hbox{\\verb.&.}\n"
          <<"\\setbox\\hlboxhash=\\hbox{\\verb.#.}\n"
          <<"\\setbox\\hlboxat=\\hbox{\\verb.@.}\n"
          <<"\\setbox\\hlboxbackslash=\\hbox{\\verb.\\.}\n"
          <<"\\setbox\\hlboxpercent=\\hbox{\\verb.\\%.}\n"
          <<"\\setbox\\hlboxhat=\\hbox{\\verb.^.}\n"

          <<"\\def\\urltilda{\\kern -.15em\\lower .7ex\\hbox{\\~{}}\\kern .04em}\n";
    }

    return os.str();
}

string LatexGenerator::getFooter()
{
    ostringstream os;
    
    if (beamerMode)
      os  << "}\n";

    os << "\\end {document}\n";
    
    if (!omitVersionComment) {
     os  << "(* LaTeX generated by highlight "
       << HIGHLIGHT_VERSION
       << ", "
       << HIGHLIGHT_URL
       << " *)\n";
    }
    return os.str();
}


void LatexGenerator::initOutputTags()
{

    openTags.push_back ( "\\hl"+STY_NAME_STD+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_STR+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_NUM+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_SLC+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_COM+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_ESC+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_DIR+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_DST+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_LIN+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_SYM+"{" );
    openTags.push_back ( "\\hl"+STY_NAME_IPL+"{" );

    for (unsigned int i=0; i<NUMBER_BUILTIN_STATES; i++ ) {
        closeTags.push_back ( "}" );
    }
}

string LatexGenerator::getAttributes ( const string & elemName,
                                       const ElementStyle &elem )
{
    ostringstream s;
    s  << "\\newcommand{\\hl"
       << elemName
       << "}[1]{\\textcolor[rgb]{"
       << elem.getColour().getRed ( LATEX ) << ","
       << elem.getColour().getGreen ( LATEX ) << ","
       << elem.getColour().getBlue ( LATEX )
       << "}{";

    if ( elem.isBold() )
        s << "\\bf{";
    if ( elem.isItalic() )
        s << "\\it{";

    s  <<"#1";

    if ( elem.isBold() )
        s << "}";
    if ( elem.isItalic() )
        s << "}";

    s  <<"}}\n";
    return s.str();
}


string LatexGenerator::getNewLine()
{
    string nl;

    // set wrapping arrow if previous line was wrapped
    if ( preFormatter.isWrappedLine ( lineNumber-1 ) ) {
        nl = "\\Righttorque";
    }
    nl += ( showLineNumbers ) ? newLineTag:longLineTag;
    return nl;
}

string LatexGenerator::maskCharacter ( unsigned char c )
{
    switch ( c ) {
    case ' ':
        return spacer;
        break;

    case '<' :
        return prettySymbols ? "\\usebox{\\hlboxlessthan}" : "$<$";
        break;
    case '>' :
        return prettySymbols ? "\\usebox{\\hlboxgreaterthan}" : "$>$";
        break;
    case '{':
        return prettySymbols ? "\\usebox{\\hlboxopenbrace}" : "\\{";
        break;
    case '}':
        return prettySymbols ? "\\usebox{\\hlboxclosebrace}" : "\\}";
        break;

    case '&':
    case '$':
    case '#':
    case '%': {
        string m ( "\\" );
        m += c;
        return m;
    }
    break;
    case '\"':
        return ( replaceQuotes ) ?"\\dq{}":"\"";
        break;
    case '_':
        return "\\textunderscore ";
        break;
    case '^':
        return "\\textasciicircum ";
        break;
    case '\\':
        return "$\\backslash$";
        break;
    case '~':
        return prettySymbols ? "\\urltilda " : "$\\sim$";
        break;
    case '|':
        return "\\textbar ";
        break;
    // avoid latex compilation failure if [ or * follows a line break (\\)
    case '*':
    case '[':
    case ']':
    // avoid "merging" of consecutive '-' chars when included in bold font ( \bf )
    case '-': {
        string m ( 1, '{' );
        m += c;
        m += '}';
        return m;
    }
    break;
    default :
        return string ( 1, c );
    }
}

string LatexGenerator::getKeywordOpenTag ( unsigned int styleID )
{
    return "\\hl"+currentSyntax->getKeywordClasses() [styleID]+"{";
}

string LatexGenerator::getKeywordCloseTag ( unsigned int styleID )
{
    return "}";
}

string LatexGenerator::getStyleDefinition()
{
    if ( styleDefinitionCache.empty() ) {
        ostringstream os;
        
        os << "% highlight theme: "<<docStyle.getDescription()<<"\n";

        os << getAttributes ( STY_NAME_STD, docStyle.getDefaultStyle() );
        os << getAttributes ( STY_NAME_NUM, docStyle.getNumberStyle() );
        os << getAttributes ( STY_NAME_ESC, docStyle.getEscapeCharStyle() );
        os << getAttributes ( STY_NAME_STR, docStyle.getStringStyle() );
        os << getAttributes ( STY_NAME_DST, docStyle.getPreProcStringStyle() );
        os << getAttributes ( STY_NAME_SLC, docStyle.getSingleLineCommentStyle() );
        os << getAttributes ( STY_NAME_COM, docStyle.getCommentStyle() );
        os << getAttributes ( STY_NAME_DIR, docStyle.getPreProcessorStyle() );
        os << getAttributes ( STY_NAME_SYM, docStyle.getOperatorStyle() );
        os << getAttributes ( STY_NAME_IPL, docStyle.getInterpolationStyle() );
        os << getAttributes ( STY_NAME_LIN, docStyle.getLineStyle() );

        KeywordStyles styles = docStyle.getKeywordStyles();
        for ( KSIterator it=styles.begin(); it!=styles.end(); it++ ) {
            os << getAttributes ( it->first, it->second );
        }
        os << "\\definecolor{bgcolor}{rgb}{"
           << docStyle.getBgColour().getRed ( LATEX ) << ","
           << docStyle.getBgColour().getGreen ( LATEX ) << ","
           << docStyle.getBgColour().getBlue ( LATEX )
           << "}\n";

        styleDefinitionCache=os.str();
    }
    return styleDefinitionCache;
}

}
