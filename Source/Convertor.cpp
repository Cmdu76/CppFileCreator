#include "Convertor.hpp"

Convertor::Convertor(std::string filename)
{
  	mInputFile = filename;
    if(!getOutputFile())
    	return;
    getClassName();
    if(!initialize())
    	return;
    run();
    stop();
}

bool Convertor::getOutputFile()
{
    size_t found = mInputFile.rfind(".");
    if (found == std::string::npos)
    {
        std::cerr << "Error with filename..." << std::endl;
        mOutputFile = "";
        return false;
    }
    else
        mOutputFile = mInputFile.substr(0, found) + ".cpp";
    return true;
}

void Convertor::getClassName()
{
    size_t found = mInputFile.rfind(".");
    if (found != std::string::npos)
        mClassName = mInputFile.substr(0, found);
    else
        mClassName = mInputFile;
    while(mClassName.find("/") != std::string::npos)
        mClassName = mClassName.substr(mClassName.find("/")+1, mClassName.size()-1);
    while(mClassName.find("\\") != std::string::npos)
        mClassName = mClassName.substr(mClassName.find("\\")+1, mClassName.size()-1);
}

std::string Convertor::getExt()
{
    size_t found = mInputFile.rfind(".");
    if (found != std::string::npos)
        return mInputFile.substr(found+1,mInputFile.size()-1);
    return "";
}

bool Convertor::initialize()
{
    mInput.open(mInputFile.c_str());
    mOutput.open(mOutputFile.c_str());

    if (!mInput || !mOutput)
    {
    	std::cerr << "Error cannot open/write file(s)..." << std::endl;
    	return false;
    }

    mOutput << "#include \"" << mClassName << "." << getExt() << "\"" << std::endl << std::endl;

    mLineCount = 0;
    mCommented = false;
    mStruct = false;
    return true;
}

void Convertor::run()
{
    while(std::getline(mInput,mTempLine))
    {
        initializeNewLoop();

        if(commentTesterOpen())
        {
            commentTesterClose();
            continue;
        }

        if(firstCheck())
            continue;
		if(handleNamespace())
	    	continue;
        if(handleStruct())
            continue;
        if(cutLine())
            continue;
        if(secondCheck())
            continue;
        handlePost();
        if(mType == Type::None)
            handleCtor();
        if(mType == Type::None)
            handleDtor();
        if(mType == Type::None)
            handleVoid();
        if(mType == Type::None)
            mType = Type::Return;

        write();
    }
}

void Convertor::initializeNewLoop()
{
    mLineCount++;
    mWords.clear();
    mType = Type::None;
    while(mTempLine.front() == ' ') mTempLine.erase(0,1);
    while(mTempLine.find("\t") != std::string::npos) mTempLine.erase(0,1);
    while(mTempLine.find("\n") != std::string::npos) mTempLine.erase(0,1);
}

bool Convertor::commentTesterOpen()
{
    if(mTempLine.find("/*") == 1 && !mCommented)
        mCommented = true;
    if(mTempLine.find("*/") == 1 && mCommented)
        mCommented = false;
    return mCommented;
}

void Convertor::commentTesterClose()
{
    if(mTempLine.find("*/") != std::string::npos && mCommented)
        mCommented = false;
}

bool Convertor::firstCheck()
{
    if(mTempLine.find("};") != std::string::npos && mStruct)
    {
        mStruct = false;
        return true;
    }

    if (mTempLine.front() == '#' || mTempLine.front() == '{' || mTempLine.front() == '}' || mTempLine.front() == '/' || mTempLine.front() == '*')
        return true;
    return false;
}

bool Convertor::handleNamespace()
{
    size_t found = mTempLine.find("namespace ");
    if(found != std::string::npos)
    {
		mOutput << mTempLine << std::endl << "{" << std::endl << std::endl;
		mNamespaces.push_back(mTempLine.erase(0,10));
		return true;
    }
    return false;
}

bool Convertor::handleStruct()
{
    if(mTempLine.find("struct ") != std::string::npos && !mStruct)
    {
		mStruct = true;
		mStructName = mTempLine.erase(0,7);
		return true;
    }
    return false;
}


bool Convertor::cutLine()
{
    size_t found = mTempLine.find(";");
    if(found == std::string::npos)
        return true;
    if(mTempLine.find("/*") != std::string::npos)
        mCommented = true;
    mTempLine = mTempLine.substr(0,found+1);
    found = mTempLine.find("(");
    if(found == std::string::npos ||(mTempLine.find(");") == std::string::npos && mTempLine.find(") const;") == std::string::npos))
        return true;
    mTempLine.pop_back();
    std::string p = mTempLine.substr(found,mTempLine.size()-1);
    handleParameters(p);
    mTempLine = mTempLine.substr(0,found);
    std::stringstream stream(mTempLine);
    std::string temp;
    while(std::getline(stream,temp,' ')) mWords.push_back(temp);
    if(p != "") mWords.push_back(p);
    if(mWords.size() < 2)
        return true;
    return false;
}

void Convertor::handleParameters(std::string& p)
{
    if(p == "()")
        return;
    while(p.find("=") != std::string::npos)
    {
        size_t found = p.find("=");
        while(p.at(found) != ',' && p.at(found) != ')')
            p.erase(found,1);
        while(p.find(" ,") != std::string::npos)
            p.erase(p.find(" ,"),1);
        while(p.find(" )") != std::string::npos)
            p.erase(p.find(" )"),1);
    }
}

bool Convertor::secondCheck()
{
    if (mWords[0] == "typedef" || mWords[0] == "enum" || mWords[0] == "public:" || mWords[0] == "public" || mWords[0] == "protected:"
     || mWords[0] == "protected" || mWords[0] == "private:" || mWords[0] == "private" || mWords[0] == "namespace" || mWords[0] == "class" || mWords[0] == "friend" || mWords[0] == "template")
        return true;

    if (mWords[0] == "virtual" && mWords[mWords.size()-1].find("0") != std::string::npos)
        return true;
    return false;
}

void Convertor::handlePost()
{
    if(mWords[0] == "static" || mWords[0] == "virtual")
         mWords.erase(mWords.begin());
    if (mWords[0] == "explicit")
    {
        mWords.erase(mWords.begin());
        mType = Type::Ctor;
    }
}

void Convertor::handleCtor()
{
    if(mWords.size() == 2 && mWords[0].find("~") == std::string::npos)
        mType = Type::Ctor;
}

void Convertor::handleDtor()
{
    if(mWords.size() == 2 && mWords[0].find("~") != std::string::npos)
        mType = Type::Dtor;
}

void Convertor::handleVoid()
{
    if(mWords.size() == 3 && mWords[0] == "void")
        mType = Type::Void;
}

void Convertor::write()
{
    if(mType != Type::None) mOutput << "////////////////////////////////////////////////////////////" << std::endl;
    if(!mStruct)
    {
        if(mType == Type::Ctor || mType == Type::Dtor)
        {
            mOutput << mClassName << "::" << mWords[0] << mWords[1] << std::endl;
            mOutput << "{" << std::endl;
            mOutput << "    " << std::endl;
            mOutput << "}" << std::endl;
        }
        if(mType == Type::Void)
        {
            mOutput << mWords[0] << " " << mClassName << "::" << mWords[1] << mWords[2] << std::endl;
            mOutput << "{" << std::endl;
            mOutput << "    " << std::endl;
            mOutput << "}" << std::endl;
        }
        if(mType == Type::Return)
        {
            std::string p = mWords.back();
            mWords.pop_back();
            std::string f = mWords.back();
            mWords.pop_back();
            for(auto w : mWords)
            {
                mOutput << w << " ";
            }
            mOutput << mClassName << "::" << f << p << std::endl;
            mOutput << "{" << std::endl;
            mOutput << "    return ;" << std::endl;
            mOutput << "}" << std::endl;
        }
    }
    else
    {
        if(mType == Type::Ctor || mType == Type::Dtor)
        {
            mOutput << mClassName << "::" << mStructName << "::" << mWords[0] << mWords[1] << std::endl;
            mOutput << "{" << std::endl;
            mOutput << "    " << std::endl;
            mOutput << "}" << std::endl;
        }
        if(mType == Type::Void)
        {
            mOutput << mWords[0] << " " << mClassName << "::" << mStructName << "::" << mWords[1] << mWords[2] << std::endl;
            mOutput << "{" << std::endl;
            mOutput << "    " << std::endl;
            mOutput << "}" << std::endl;
        }
        if(mType == Type::Return)
        {
            std::string p = mWords.back();
            mWords.pop_back();
            std::string f = mWords.back();
            mWords.pop_back();
            for(auto w : mWords)
            {
                mOutput << w << " ";
            }
            mOutput << mClassName << "::"  << mStructName << "::" << f << p << std::endl;
            mOutput << "{" << std::endl;
            mOutput << "    return ;" << std::endl;
            mOutput << "}" << std::endl;
        }
    }
	if(mType != Type::None)
        mOutput << std::endl;
}

void Convertor::stop()
{
    for(std::string n : mNamespaces)
		mOutput << "} // namespace " << n << std::endl << std::endl;

	mInput.close();
    mOutput.close();
    std::cout << "File created !" << std::endl;
}
