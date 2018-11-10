// resizeobj.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include "utils.h"
#include "paknode.h"
#include "ImgConverter.h"
#include "VhclConverter.h"
#include "TileConverter.h"

const char *my_signature = "/resizeobj";

class ResizeObj
{
public:
	ResizeObj()
	{
		m_tc.imgConverter(&m_ic);
	};

	void convertFilemask(std::string filemask) const;

	void antialias(int val){ m_ic.alpha(val); };
	void addonPrefix(std::string val){ m_addonPrefix = val; };
	void newExtension(std::string val){ m_newExt = val; };
	void headerRewriting(bool val){ m_headerRewriting = val;};
	void keepImageSize(bool val){ m_keepImageSize = val; };
	void lengthMode(LengthMode val){ m_vc.lengthMode(val); };
	void specialColorMode(SCConvMode val){ m_ic.specialColorMode(val); };
	void tileNoAnimation(bool val){ m_tc.noAnimation(val); };
private:
	ImgConverter m_ic;
	VhclConverter m_vc;
	TileConverter m_tc;
	std::string m_addonPrefix;
	bool m_headerRewriting;
	bool m_keepImageSize;
	std::string m_newExt;

	void convertFile(std::string filename) const;
	void convertPak(PakFile &pak) const;
	void convertAddon(PakNode *addon) const;
};

/// アドオン名変更処理の対象外とする形式一覧.
const char *renameNodeExceptions[] = {
	"GOOD",
//	"SMOK",
};


bool isRenameNode(const char *name)
{
	for(int i = 0; i<lengthof(renameNodeExceptions); i++)
	{
		if(strncmp(renameNodeExceptions[i], name, NODE_NAME_LENGTH)==0)
			return false;
	}
	return true;
}

/**
	アドオン名の先頭にposfixを追加する.

	アドオン名定義用のTEXTノードとアドン参照用のXREFノードの先頭にprefixを追加する。
書籍:
pak64:  Buecher
pak128: Bucher
*/
void renameobj(PakNode *node, std::string prefix)
{
	if(!isRenameNode(node->type().data()))
		return;

	// XREFノードの場合は変換する
	if(node->type()=="XREF")
	{
		PakXRef *x = &node->data_p()->xref;

		if(isRenameNode(&x->type[0]))
		{
			if(node->data()->size()>sizeof(PakXRef))
			{
				std::vector<char> *dat = node->data();
				dat->insert(dat->begin() + offsetof(PakXRef, name),
					prefix.begin(), prefix.end()); 
			}
		}
	}	
	
	// 最初の子ノードがTEXTの場合はそれをアドオン名とみなして変換する。
	if(node->begin() != node->end() && (*node->begin())->type()=="TEXT")
	{
		std::vector<char> *dat = (*node->begin())->data();
		dat->insert(dat->begin(), prefix.begin(), prefix.end()); 
	}

	for(PakNode::iterator it = node->begin(); it != node->end(); it++) renameobj(*it, prefix);
}


void ResizeObj::convertAddon(PakNode *addon) const
{
	std::string name = nodeStrings(addon->begin(), addon->end());
	printf("  アドオン -- %s:\n", name.c_str());

	if(m_keepImageSize)
	{
		if(addon->type() == "VHCL")
		{
			printf("    乗り物の画像表示位置を調整します。\n");
			m_vc.convertVhclAddon(addon);
		}
		else if(addon->type() == "SMOK")
		{
			printf("    煙の画像表示位置を調整します。\n");
			m_tc.convertAddon(addon);
		}
		else if(addon->type() == "BUIL" || addon->type() == "FACT"
			  || addon->type() == "FIEL")
		{
			printf("    建築物のタイル数を拡大します。\n");
			m_tc.convertAddon(addon);
		}
	}else{
		printf("    画像を縮小します。\n");
		m_ic.convertAddon(addon);
	}

	if(m_addonPrefix.size())
	{
		printf("    アドオン名を変更します。\n");
		renameobj(addon, m_addonPrefix);
	}
}

void ResizeObj::convertPak(PakFile &pak) const
{
	if(m_headerRewriting)
		pak.signature(pak.signature() + my_signature);

	PakNode &root = pak.root();
	for(PakNode::iterator it = root.begin(); it != root.end(); it++)
		convertAddon(*it);
}

void ResizeObj::convertFile(std::string filename) const
{
	puts("====================================");
	printf("ファイル -- %s:\n", filename.c_str());

	PakFile pak;
	pak.loadFromFile(filename);

	if (pak.signature().find(my_signature)!=std::string::npos)
	{
		puts("  スキップしました.");
		return;
	}

	convertPak(pak);

	if(m_newExt != "")
		filename = changeFileExt(filename, m_newExt);

#ifdef _DEBUG
	// 「-E=NUL」でファイル出力無し
	if(m_newExt == "NUL") return;
#endif
	pak.saveToFile(filename);
}

void ResizeObj::convertFilemask(std::string filemask) const
{
	if (filemask.find_first_of("*?")==std::string::npos)
	{
		convertFile(filemask);
	}else{
		std::vector<std::string> files;
		fileList(files, filemask);
		for(std::vector<std::string>::iterator it = files.begin(); it != files.end(); it++)
			convertFile(*it);
	}
}


void printOption()
{
	puts(
#ifdef _DEBUG
	"DEBUG "
#endif
	"resizeobj ver 1.1.0 beta by wa\n\n"
	"resizeobj [オプション...] 対象ファイルマスク...\n"
	"オプション:\n\n"
	" -A=《0～100》画像縮小時のアンチエイリアス量\n"
	"                0: アンチエイリアス無し\n"
	"              100: アンチエイリアス強め(既定値)\n"
	" -S=《0～2》  画像縮小時の特殊色(発光色・プレイヤー色)の扱い\n"
	"                0: 特殊色を使用せず縮小し、昼間の見た目を優先する\n"
	"                1: 縮小元エリアの左上が特殊色の場合はそれを出力(既定値)\n"
	"                2: 縮小元エリアで特殊色が半数以上使用されている場合はそれを出力\n"
	"\n"
	" -K           原寸モード\n"
	" -KA          原寸モードで建築物を変換する際にアニメーションを取り除く\n"
	" -L           原寸モードで乗り物を変換する際にlength属性を変換しない\n"
	"\n"
	" -E=《拡張子》出力するファイルの拡張子。既定値は「.64.pak」\n"
	" -T=《text》  アドオン名の先頭に指定されたtextを追加する\n"
	" -N           ファイルヘッダの書き換えを行わない\n"
	"\n"
	" -D           エラー時にダイアログを表示しない\n"
	" -? , -H      この画面を出力する\n"
	"");
}

template<class T> T optToEnum(const std::string &text, int high, const char *opt)
{
	int value = atoi(text.c_str());
	if (value<0 || high<value)
	{
		std::ostringstream os;
		os << opt << "オプションの有効範囲は0～" << high << "です。: " << text;
		throw std::runtime_error(os.str());
	}
	return static_cast<T>(value);
}

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
#endif

	bool showErrorDialog = true;

	try
	{
		std::locale::global(std::locale(""));

		ResizeObj ro;
		ro.antialias(100);
		ro.headerRewriting(true);
		ro.lengthMode(STRICT_CONVERT_LENGTH);
		ro.specialColorMode(scmTOPLEFT);
		ro.addonPrefix("");
		ro.keepImageSize(false);
		ro.newExtension(".64.pak");
		ro.tileNoAnimation(false);

		std::vector<std::string> args;
		for(int i=1; i<argc; i++)
		{
			std::string key, val;
			parseArg(argv[i], key, val);
			if(key==""){ args.push_back(val);}
			else if(key=="K"){ ro.keepImageSize(true); }
			else if(key=="KA"){ ro.tileNoAnimation(true); }
			else if(key=="A"){ ro.antialias(optToEnum<int>(val, 100, "A"));}
			else if(key=="S"){ ro.specialColorMode(optToEnum<SCConvMode>(val, 2, "S")); }
			else if(key=="N"){ ro.headerRewriting(false);    }
			else if(key=="T"){ ro.addonPrefix(val);        }
			else if(key=="L"){ ro.lengthMode(NO_CONVERT_LENGTH); }
			else if(key=="E"){ if(val!="") ro.newExtension(val); }
			else if(key=="D"){ showErrorDialog = false; }
			else if(key=="H" || key=="?")
			{
				printOption();
				return 0;
			}
			else{
				printf("無効なスイッチです : \"%s\"\n\n", key.c_str());
				printOption();
				return 1;
			}
		}
		if(args.empty())
		{
			printOption();
			return 0;
		}

		for(std::vector<std::string>::iterator it = args.begin(); it != args.end(); it++)
			ro.convertFilemask(*it);

	}catch(const std::exception &e)
	{
		printf("\nエラー: %s\n", e.what());
		if(showErrorDialog)
			MessageBox(0, e.what(), "エラー", MB_OK | MB_APPLMODAL | MB_ICONERROR);
		return 1;
	}		
	return 0;
}

