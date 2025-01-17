// ==========  This file is under  LGPL, the GNU Lesser General Public License
// ==========  Dialing Lemmatizer (www.aot.ru)
// ==========  Copyright by Alexey Sokirko
#include "Lemmatizers.h"
#include "morph_dict/common/utilit.h"
#include "Paradigm.h"
#include "rus_numerals.h"
#include <fstream>


CLemmatizer::CLemmatizer(MorphLanguageEnum Language) : 
	CMorphDict(Language), 
	m_bEnablePrediction(true),
	m_Predict(Language)
{	
	m_bLoaded = false;
	m_bUseStatistic = false;
	m_bMaximalPrediction = false;
	m_bAllowRussianJo = false;
	InitAutomat( new CMorphAutomat(Language, MorphAnnotChar) );
};

CLemmatizer::~CLemmatizer()
{
};


std::string GetPathByLanguage(MorphLanguageEnum langua)
{
	auto key = Format("Software\\Dialing\\Lemmatizer\\%s\\DictPath", GetStringByLanguage(langua).c_str());
	std::string load_path = ::GetRegistryString(key);
	if (	(load_path.length() > 0)	
		&&	(load_path[load_path.length() - 1] != '\\')
		&&	(load_path[load_path.length() - 1] != '/')
		)
		load_path += "/";

	return load_path;
};;


bool CLemmatizer::CheckABC(const std::string& WordForm) const
{
	return m_pFormAutomat->CheckABCWithoutAnnotator(WordForm);
};

bool CLemmatizer::CreateParadigmFromID(uint32_t id, CFormInfo& Result) const 
{
	Result.AttachLemmatizer(this);
	return Result.SetParadigmId(id);
}

bool CLemmatizer::IsHyphenPostfix(const std::string& Postfix) const
{
	return m_HyphenPostfixes.find(Postfix) != m_HyphenPostfixes.end();
};

bool CLemmatizer::IsHyphenPrefix(const std::string& Prefix) const
{
	return m_HyphenPrefixes.find(Prefix) != m_HyphenPrefixes.end();
};

const CStatistic& CLemmatizer::GetStatistic() const 
{	
	return m_Statistic;	
}

bool CLemmatizer::IsPrefix(const std::string& Prefix) const
{
	return m_PrefixesSet.find(Prefix) != m_PrefixesSet.end();

};

//   function should return true if 
// the word was found in the dictionary, if it was predicted, then it returns false
bool CLemmatizer::LemmatizeWord(std::string& word_str, const bool cap, const bool predict, std::vector<CAutomAnnotationInner>& results, bool bGetLemmaInfos) const
{
	RmlMakeUpper (word_str, GetLanguage());

	size_t WordOffset = 0;
	

	m_pFormAutomat->GetInnerMorphInfos(word_str, 0, results);

	bool bResult = !results.empty();

	if (results.empty())
	{
		if (predict)
		{
			PredictBySuffix(word_str, WordOffset, 4, results); // the length of the minal suffix is 4 


			if (word_str[WordOffset-1] != '-') //  and there is no hyphen
			{
				size_t  KnownPostfixLen = word_str.length() - WordOffset;
				size_t  UnknownPrefixLen = WordOffset;
				if (KnownPostfixLen < 6)// if  the known part is too short
					//if	(UnknownPrefixLen > 5)// no prediction if unknown prefix is more than 5
					{
						if (!IsPrefix(word_str.substr(0, UnknownPrefixLen)))
							results.clear();
					};
			};

			// отменяем предсказание по местоимениям, например _R("Семыкиным")
			for (size_t i=0; i<results.size(); i++)
				if (!m_ProductiveModels[results[i].m_ModelNo])
				{
					results.clear();
					break;
				};

		};
	};

	if (!results.empty())
	{
		if (bGetLemmaInfos)
			GetLemmaInfos(word_str, WordOffset, results);
	}
	else
		if (predict)
		{
			PredictByDataBase(word_str, results,cap);
			for (int i=(int)results.size()-1; i>=0; i--)
			{
					const CAutomAnnotationInner& A = results[i];
					const CLemmaInfo& I = m_LemmaInfos[A.m_LemmaInfoNo].m_LemmaInfo;
					const CFlexiaModel& M = m_FlexiaModels[A.m_ModelNo];
					const CMorphForm& F = M.m_Flexia[A.m_ItemNo];
					if ( F.m_FlexiaStr.length() >= word_str.length() )
					{
						results.erase(results.begin() + i);
					}
			}
		};

	return bResult;
}

bool CLemmatizer::GetAllAncodesAndLemmasQuick(std::string& word_str, bool capital, char* OutBuffer, size_t MaxBufferSize, bool bUsePrediction) const
{
	// word_str is in single-byte encoding
	FilterSrc(word_str);

	std::vector<CAutomAnnotationInner>	FindResults;

	bool bFound = LemmatizeWord(word_str, capital, bUsePrediction, FindResults, false);

	size_t Count = FindResults.size();
	size_t  OutLen = 0;
	for (uint32_t i=0; i < Count; i++)
	{
		const CAutomAnnotationInner& A = FindResults[i];
		const CFlexiaModel& M = m_FlexiaModels[A.m_ModelNo];
		const CMorphForm& F = M.m_Flexia[A.m_ItemNo];
		size_t PrefixLen = F.m_PrefixStr.length();
		size_t BaseStart  = 0;
		if	(		bFound 
				||	!strncmp(word_str.c_str(), F.m_PrefixStr.c_str(), PrefixLen)
			)
			BaseStart = PrefixLen;
		int BaseLen  = (int)word_str.length() - (int)F.m_FlexiaStr.length() - (int)BaseStart;
		if (BaseLen < 0) BaseLen = (int)word_str.length();
		size_t GramCodeLen = M.m_Flexia[A.m_ItemNo].m_Gramcode.length();
		size_t FlexiaLength = M.m_Flexia[0].m_FlexiaStr.length();
		if (BaseLen+FlexiaLength+3+GramCodeLen > MaxBufferSize-OutLen) return false; 

		strncpy(OutBuffer+OutLen, word_str.c_str()+BaseStart, BaseLen);
		OutLen += BaseLen;

		strncpy(OutBuffer+OutLen, M.m_Flexia[0].m_FlexiaStr.c_str(), FlexiaLength);
		OutLen += FlexiaLength;

		OutBuffer[OutLen] = ' ';
		OutLen++;

		strncpy(OutBuffer+OutLen, M.m_Flexia[A.m_ItemNo].m_Gramcode.c_str(), GramCodeLen);
		OutLen += GramCodeLen+1;
		OutBuffer[OutLen-1] = 	'#';

	};
	OutBuffer[OutLen] = 0;
	return true;
	
}

void CLemmatizer::ReadOptions(std::string file_path)
{
	LOGV << "load " << file_path;
	std::ifstream inp(file_path);
	rapidjson::IStreamWrapper isw(inp);
	rapidjson::Document d;
	d.ParseStream(isw);
	inp.close();

	auto p = rapidjson::Pointer("/AllowRussianJo").Get(d);
	m_bAllowRussianJo = (p) ? p->GetBool() : false;
	p = rapidjson::Pointer("/SkipPredictBase").Get(d);
	if (p && p->GetBool()) {
		m_bEnablePrediction = false;
	};
};


void CLemmatizer::LoadDictionariesFromPath(std::string load_path)
{
	LOGV << "load " << load_path;
	Load(MakePath(load_path, MORPH_MAIN_FILES));
	m_bLoaded = true;

	LOGV << "load literature statistics " << MakePath(load_path, "l");
	m_Statistic.Load(MakePath(load_path, "l"));

	m_bUseStatistic = true;

	ReadOptions(MakePath(load_path, OPTIONS_FILE));

	if (m_bEnablePrediction) {
		auto predict_path = MakePath(load_path, PREDICT_BIN_PATH);
		LOGV << "load " << predict_path;
		m_Predict.Load(predict_path);

		m_Predict.m_ModelFreq.resize(m_FlexiaModels.size(), 0);
		for (const auto& l : m_LemmaInfos) {
			++m_Predict.m_ModelFreq[l.m_LemmaInfo.m_FlexiaModelNo];
		}
	};
	m_PrefixesSet.clear();
	m_PrefixesSet.insert(m_Prefixes.begin(), m_Prefixes.end());
}

void CLemmatizer::LoadDictionariesRegistry()
{
	LoadDictionariesFromPath(GetPathByLanguage(m_Language));
}


bool IsFound(const std::vector<CFormInfo> & results)
{
    return !results.empty() && results[0].m_bFound;
};

void CreateDecartProduction (const std::vector<CFormInfo>& results1, const std::vector<CFormInfo>& results2, std::vector<CFormInfo>& results)
{
	results.clear();
	for (size_t i=0; i<results1.size(); i++)
		for (size_t k=0; k<results2.size(); k++)
		{
            CFormInfo F = results2[k];
            F.SetUserPrefix(results1[i].GetWordForm(0)+"-");
			results.push_back(F);
		};

};

void CLemmatizer::predict_hyphen_word(std::string& wordform, bool capital, std::vector<CFormInfo>& Result) const {
    size_t hyph = wordform.find("-");
    if (hyph == std::string::npos)  {
        return;
    }

    std::vector<CFormInfo> results1, results2;
    bool gennum = false;
    // try to lemmatize each parts without predictions
    std::string first_part = wordform.substr(0, hyph);
    std::string second_part = wordform.substr(gennum ? hyph : hyph+1);
    CreateParadigmCollection(false, first_part, capital, false, results1 );

    /*
     if the first part is equal to the second part  or the second part is an unimportant: Russian postfix
     then we should use only the first part
    */
    if	(			(first_part == second_part)
                       ||		IsHyphenPostfix(second_part)
            )
        Result = results1;

    else
    if (IsHyphenPrefix(first_part))
    {
        CreateParadigmCollection(false, second_part, capital,  false, results2 );
        if (IsFound(results2))
        {
            Result = results2;
            for (int i=0; i < Result.size(); i++)
            {
                Result[i].SetUserPrefix(first_part+"-");
                Result[i].SetUserUnknown();

            }
        }
    }
    else
    {
        CreateParadigmCollection(false,second_part, false, false, results2 );
        if (IsFound(results1) && IsFound(results2) && first_part.length()>2  && second_part.length()>2)
        {
            // if both words were found in the dictionary
            // then we should create a decart production
            CreateDecartProduction(results1, results2, Result);
            for (auto& r: Result)
                r.SetUserUnknown();
        }
    }
}

bool CLemmatizer::CreateParadigmCollection(bool bNorm, std::string& word_str, bool capital, bool bUsePrediction, std::vector<CFormInfo>& Result) const
{
    Result.clear();
	FilterSrc(word_str);
	std::vector<CAutomAnnotationInner>	found;
	bool bFound = LemmatizeWord(word_str, capital, bUsePrediction, found, true);
		
	for (auto& a: found)
	{
		// if bNorm, then  ignore words which are not lemma
		if (bNorm && (a.m_ItemNo!=0)) continue;

		if (m_bUseStatistic)
			a.m_nWeight = m_Statistic.get_HomoWeight(a.GetParadigmId(), a.m_ItemNo);
		else
			a.m_nWeight = 0;

		CFormInfo P;
		P.Create(this, a, word_str, bFound);
		Result.push_back(P);
	}

	if (!IsFound(Result)) // not found or predicted
	{
		// if the word was not found in the dictionary 
		// and the word contains a hyphen 
		// then we should try to predict each parts of the hyphened word separately
        predict_hyphen_word(word_str, capital, Result);
	};
	return true;
}


bool CLemmatizer::LoadStatisticRegistry(SubjectEnum subj)
{
	try
	{
		std::string load_path = GetPathByLanguage(m_Language);
		std::string prefix;
		switch (subj)
		{
		case subjFinance:
			prefix += "f";
			break;
		case subjComputer:
			prefix += "c";
			break;
		case subjLiterature:
			prefix += "l";
			break;
		default:
			return false;
		}
		m_Statistic.Load(load_path + prefix);
		return true;
	}
	catch(...)
	{
		return false;
	}
}

CAutomAnnotationInner  CLemmatizer::ConvertPredictTupleToAnnot(const CPredictTuple& input) const  
{
	CAutomAnnotationInner node;
	node.m_LemmaInfoNo = input.m_LemmaInfoNo;
	node.m_ModelNo = m_LemmaInfos[node.m_LemmaInfoNo].m_LemmaInfo.m_FlexiaModelNo;
	node.m_nWeight = 0;
	node.m_PrefixNo = 0;
	node.m_ItemNo = input.m_ItemNo;
	return node;
};


bool CLemmatizer::CheckAbbreviation(std::string InputWordStr,std::vector<CAutomAnnotationInner>& FindResults, bool is_cap) const
{
	for(size_t i=0; i <InputWordStr.length(); i++)
		if (!is_upper_consonant((BYTE)InputWordStr[i], GetLanguage()))
			return false;

	std::vector<CPredictTuple> res;
	m_Predict.Find(m_pFormAutomat->GetCriticalNounLetterPack(),res); 
	FindResults.push_back(ConvertPredictTupleToAnnot(res[0]));
	return true;
};

void CLemmatizer::PredictByDataBase(std::string InputWordStr,  std::vector<CAutomAnnotationInner>& FindResults,bool is_cap) const  
{

	std::vector<CPredictTuple> res;
	if (CheckAbbreviation(InputWordStr, FindResults, is_cap))
		return;

	if (CheckABC(InputWordStr)) // if the ABC is wrong this prediction yuilds to many variants
	{		
		reverse(InputWordStr.begin(),InputWordStr.end());
		m_Predict.Find(InputWordStr,res);
	}

	std::vector<int> has_nps(32, -1); // assume not more than 32 different pos
	for( int j=0; j<res.size(); j++ )
	{
		BYTE PartOfSpeechNo = res[j].m_PartOfSpeechNo;
		if	(		!m_bMaximalPrediction 
				&& (has_nps[PartOfSpeechNo] != -1) 
			)
		{
			int old_freq = m_Predict.m_ModelFreq[FindResults[has_nps[PartOfSpeechNo]].m_ModelNo];
			int new_freq = m_Predict.m_ModelFreq[m_LemmaInfos[res[j].m_LemmaInfoNo].m_LemmaInfo.m_FlexiaModelNo];
			if (old_freq < new_freq)
				FindResults[has_nps[PartOfSpeechNo]] = ConvertPredictTupleToAnnot(res[j]);
			
			continue;
		};

		has_nps[PartOfSpeechNo] = (BYTE)FindResults.size();

		FindResults.push_back(ConvertPredictTupleToAnnot(res[j]));
	}

	if	(		(has_nps[0] == -1) // no noun
			||	( is_cap && (GetLanguage() != morphGerman)) // or can be a proper noun (except German, where all nouns are written uppercase)
		)
	{
		m_Predict.Find(m_pFormAutomat->GetCriticalNounLetterPack(),res); 
		FindResults.push_back(ConvertPredictTupleToAnnot(res.back()));
	};

}

bool CLemmatizer::IsInDictionary(std::string& word_s8, const bool cap) const {
    std::vector<CAutomAnnotationInner> results;
	FilterSrc(word_s8);
    return LemmatizeWord(word_s8, cap, false, results, false);
}

std::vector<CFuzzyResult> CLemmatizer::CorrectMisspelledWord1(std::string word_s8, size_t maxStrDistance) const
{
	RmlMakeUpper(word_s8, GetLanguage());
	auto res = m_pFormAutomat->FuzzySearch(word_s8, maxStrDistance);
	for (int i = 0; i < res.size(); ++i) {
		if (res[0] < res[i]) {
			res.erase(res.begin() + i, res.end());
			break;
		}
	}
	return res;
}


CLemmatizerRussian::CLemmatizerRussian() : CLemmatizer(morphRussian)
{
	m_HyphenPostfixes.insert(_R("КА"));
	m_HyphenPostfixes.insert(_R("ТО"));
	m_HyphenPostfixes.insert(_R("С"));

    m_HyphenPrefixes.insert(_R("ПОЛУ"));
    m_HyphenPrefixes.insert(_R("ПОЛ"));
    m_HyphenPrefixes.insert(_R("ВИЦЕ"));
    m_HyphenPrefixes.insert(_R("МИНИ"));
    m_HyphenPrefixes.insert(_R("КИК"));
};


void CLemmatizerRussian::FilterSrc(std::string& src) const	
{
	if (!m_bAllowRussianJo)
		ConvertJO2Je(src); 

	// переводим ' в _R("ъ"), например, "об'явление" -> _R("объявление")
	size_t len = src.length();
	for (size_t i=0; i<len; i++)
		if (src[i] == '\'')
			src[i] = _R("ъ")[0];
};

CLemmatizerEnglish:: CLemmatizerEnglish() : CLemmatizer(morphEnglish)
{
};

CLemmatizerGerman:: CLemmatizerGerman() : CLemmatizer(morphGerman)
{
};



