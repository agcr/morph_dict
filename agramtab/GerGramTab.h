// ==========  This file is under  LGPL, the GNU Lesser General Public License
// ==========  Dialing Lemmatizer (www.aot.ru)
// ==========  Copyright by Alexey Sokirko

#pragma once 

#include "agramtab.h"       


enum GermanPartOfSpeechEnum
        {
                gART = 0,
                gADJ = 1,
                gADV = 2,
                gEIG = 3,
                gSUB = 4,
                gVER = 5,
                gPA1 = 6,
                gPA2 = 7,
                gPRONOMEN = 8,
                gPRP = 9,
                gKON = 10,
                gNEG = 11,
                gINJ = 12,
                gZAL = 13,
                gZUS = 14,
                gPRO_BEG = 15,
                gZU_INFINITIV = 16,
                GERMAN_PART_OF_SPEECH_COUNT = 17
        };


enum GermanGrammemsEnum {
    // unknown 0..3
    gNoaUnk = 0,
    gPredikBenutz = 1,
    gProUnk = 2,
    gTmpUnk = 3,


    // eigennamen 4..14
    gNac = 4,
    gMou = 5,
    gCou = 6,
    gGeo = 7,
    gWasser = 8,
    gGeb = 9,
    gStd = 10,
    gLok = 11,
    gVor = 12,

    //  reflexive Verben
    gSichAcc = 13,
    gSichDat = 14,



    // verb clasess 15..18
    gSchwach = 15,
    gNichtSchwach = 16,
    gModal = 17,
    gAuxiliar = 18,


    // verb forms 19..26
    gKonj1 = 19,
    gKonj2 = 20,
    gPartizip1 = 21,
    gPartizip2 = 22,
    gZuVerbForm = 23,
    gImperativ = 24,
    gPraeteritum = 25,
    gPrasens = 26,

    //adjective 27..29
    gGrundform = 27,
    gKomparativ = 28,
    gSuperlativ = 29,

    // konjunk 30..34
    gProportionalKonjunktion = 30,
    gInfinitiv = 31, // used also for verbs
    gVergleichsKonjunktion = 32,
    gNebenordnende = 33,
    gUnterordnende = 34,



    //pronouns 35..41
    gPersonal = 35,
    gDemonstrativ = 36,
    gInterrogativ = 37,
    gPossessiv = 38,
    gReflexiv = 39,
    gRinPronomen = 40,
    gAlgPronomen = 41,

    //adjective's articles 42.44
    gAdjektiveOhneArtikel = 42,
    gAdjektiveMitUnbestimmte = 43,
    gAdjektiveMitBestimmte = 44,



    //persons 44..47
    gErstePerson = 45,
    gZweitePerson = 46,
    gDrittePerson = 47,

    //genus 48..50
    gFeminin = 48,
    gMaskulin = 49,
    gNeutrum = 50,



    // number 51..52
    gPlural = 51,
    gSingular = 52,


    //cases 53..56
    gNominativ = 53,
    gGenitiv = 54,
    gDativ = 55,
    gAkkusativ = 56,

    // abbreviation
    gAbbreviation = 57,

    //Einwohnerbezeichnung
    gEinwohner = 58,

    //
    gTransitiv = 59,
    gIntransitiv = 60,
    gImpersonal = 61,
    GermanGrammemsCount = 62

};

enum GermanClauseTypeEnum {
    VERBSATZ_T = 0,
    PARTIZIPIALSATZ_T = 1,
    INFINITIVSATZ_T = 2,
    GERMAN_CLAUSE_TYPE_COUNT = 3
};


class CGerGramTab : public CAgramtab
{
	const static size_t gStartUp = 0x4141; //AA 
	const static size_t gEndUp = 0x7A7B;  //zz + 1
	const static size_t gMaxGrmCount = gEndUp - gStartUp; // // 5911  (5 Кб) 
public:
	CAgramtabLine*  Lines[gMaxGrmCount];
	CGerGramTab();
	~CGerGramTab();

    void LoadFromRegistry() override;

	part_of_speech_t GetPartOfSpeechesCount() const;
	const char* GetPartOfSpeechStr(part_of_speech_t i, NamingAlphabet na = naDefault) const;
	const char* GetPartOfSpeechStrLong(part_of_speech_t i) const override;
	grammem_t GetGrammemsCount()  const;
	const char* GetGrammemStr(size_t i, NamingAlphabet na = naDefault) const;
	size_t GetMaxGrmCount() const;
	CAgramtabLine*& GetLine(size_t LineNo) { return Lines[LineNo]; }
	const CAgramtabLine* GetLine(size_t LineNo) const {return Lines[LineNo];}
	size_t GramcodeToLineIndex(const char * s ) const { return  (unsigned char) s[0]*0x100+(unsigned char) s[1] - gStartUp;};

	std::string LineIndexToGramcode(uint16_t i) const
	{ 
		i += gStartUp;
		char res[3];

		res[0] = (i >> 8);
		res[1] = (0xFF & i);
		res[2] = 0;
		return  res;
	};
	const char* GetRegistryString() const 
	{
		return "Software\\Dialing\\Lemmatizer\\German\\Ggramtab";
	};
	long GetClauseTypeByName(const char* TypeName) const;
	const char* GetClauseNameByType(long type) const;
	bool GleicheGenderNumber(const char* gram_code1, const char* gram_code2) const;
	bool GleicheSubjectPredicate(const char* gram_code1, const char* gram_code2) const;
	const size_t GetClauseTypesCount() const;
	bool IsStrongClauseRoot(const part_of_speech_mask_t poses) const;
    bool IsMorphNoun (part_of_speech_mask_t poses)  const;
	bool is_morph_adj (part_of_speech_mask_t poses) const;
	bool is_morph_participle (part_of_speech_mask_t poses) const;
	bool is_morph_pronoun (part_of_speech_mask_t poses) const;
	bool is_morph_pronoun_adjective(part_of_speech_mask_t poses) const;
	bool is_left_noun_modifier  (part_of_speech_mask_t poses, grammems_mask_t grammems) const;
	bool is_numeral (part_of_speech_mask_t poses) const;
	bool is_verb_form (part_of_speech_mask_t poses) const;
	bool is_infinitive(part_of_speech_mask_t poses) const;
	bool is_morph_predk(part_of_speech_mask_t poses) const;
	bool is_morph_adv(part_of_speech_mask_t poses) const;
	bool is_morph_personal_pronoun (part_of_speech_mask_t poses, grammems_mask_t grammems) const;

    bool IsSimpleParticle(const std::string& lemma, part_of_speech_mask_t poses) const;
	bool IsSynNoun(part_of_speech_mask_t poses, const std::string& lemma) const;
	bool IsStandardParamAbbr (const char* WordStrUpper) const;
	bool GleicheCase(const char* gram_code_noun, const char* gram_code_adj) const;
	bool GleicheCaseNumber(const char* gram_code1, const char* gram_code2) const;
	grammems_mask_t GleicheGenderNumberCase(const char* common_gram_code_noun, const char* gram_code_noun, const char* gram_code_adj) const;
	bool PartOfSpeechIsProductive(part_of_speech_t p) const override;
};

const grammems_mask_t gAllCases = (_QM(gNominativ) | _QM(gGenitiv) | _QM(gDativ) | _QM(gAkkusativ));
const grammems_mask_t gAllNumbers = (_QM(gPlural) | _QM(gSingular));
const grammems_mask_t gAllGenders = (_QM(gFeminin) | _QM(gMaskulin) | _QM(gNeutrum));
const grammems_mask_t gAllPersons = (_QM(gErstePerson) | _QM(gZweitePerson) | _QM(gDrittePerson));
const grammems_mask_t gAllVerbForms = (_QM(gKonj1) | _QM(gKonj2) | _QM(gImperativ) | _QM(gPrasens) | _QM(gPraeteritum));
const grammems_mask_t gAllVerbClasses = (_QM(gModal) | _QM(gAuxiliar) | _QM(gSchwach) | _QM(gNichtSchwach));

