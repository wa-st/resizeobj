// resizeobj.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <fcntl.h>
#include <io.h>

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

	void convertStdIO() const;
	void convertFile(std::string filename) const;

	void antialias(int val){ m_ic.alpha(val); };
	void addonPrefix(std::string val){ m_addonPrefix = val; };
	void newExtension(std::string val){ m_newExt = val; };
	void headerRewriting(bool val){ m_headerRewriting = val;};
	void keepImageSize(bool val){ m_keepImageSize = val; };
	void specialColorMode(SCConvMode val){ m_ic.specialColorMode(val); };
	void tileNoAnimation(bool val){ m_tc.noAnimation(val); };
	void newTileSize(int val){ m_ic.newTileSize(val); };
private:
	ImgConverter m_ic;
	VhclConverter m_vc;
	TileConverter m_tc;
	std::string m_addonPrefix;
	bool m_headerRewriting;
	bool m_keepImageSize;
	std::string m_newExt;

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


std::string addonName(PakNode *node)
{
	if(node->type()=="TEXT")
		return node->data_p()->text;
	else if(node->length()>0)
		return addonName((*node)[0]);
	else	
		return "";
}



void ResizeObj::convertAddon(PakNode *addon) const
{
	std::clog << "    " << addonName(addon) << std::endl;

	if(m_keepImageSize)
	{
		if(addon->type() == "VHCL")
		{
			m_vc.convertVhclAddon(addon, m_ic.newTileSize());
		}
		else if(addon->type() == "SMOK")
		{
			m_tc.convertAddon(addon);
		}
		else if(addon->type() == "BUIL" || addon->type() == "FACT"
			  || addon->type() == "FIEL")
		{
			m_tc.convertAddon(addon);
		}
	}else{
		m_ic.convertAddon(addon);
	}

	if(m_addonPrefix.size())
	{
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

void ResizeObj::convertStdIO() const
{
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);

	PakFile pak;
	pak.load(std::cin);
	convertPak(pak);
	pak.save(std::cout);
}

void ResizeObj::convertFile(std::string filename) const
{
	std::clog << filename << std::endl;


	PakFile pak;
	pak.loadFromFile(filename);

	if (pak.signature().find(my_signature)!=std::string::npos)
	{
		std::clog << "    skipped." << std::endl;
		return;
	}

	convertPak(pak);

	if(m_newExt != "") filename = changeFileExt(filename, m_newExt);

	pak.saveToFile(filename);
}

void printOption()
{
	std::clog << 
#ifdef _DEBUG
	"DEBUG "
#endif
	"resizeobj ver 1.2.0 beta by wa\n\n"
	"resizeobj [オプション...] 対象ファイルマスク...\n"
	"オプション:\n\n"
	" -A=(0...100) 画像縮小時のアンチエイリアス量\n"
	"                0: アンチエイリアス無し\n"
	"              100: アンチエイリアス強め(既定値)\n"
	" -S=(0...2)   画像縮小時の特殊色(発光色・プレイヤー色)の扱い\n"
	"                0: 特殊色を使用せず縮小し、昼間の見た目を優先する\n"
	"                1: 縮小元エリアの左上が特殊色の場合はそれを出力(既定値)\n"
	"                2: 縮小元エリアで特殊色が半数以上使用されている場合はそれを出力\n"
	" -W=(16...255) 画像変換後のタイルサイズを指定する。既定値は「64」"
	"\n"
	" -K           原寸モード\n"
	" -KA          原寸モードで建築物を変換する際にアニメーションを取り除く\n"
	"\n"
	" -E=(ext)     出力するファイルの拡張子。既定値は「.64.pak」\n"
	" -T=(text)    アドオン名の先頭に指定されたtextを追加する\n"
	" -N           ファイルヘッダの書き換えを行わない\n"
	"\n"
	" -D           エラー時にダイアログを表示しない\n"
	" -? , -H      この画面を出力する"
	<< std::endl;
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

	bool isShowErrorDialog = true;

	try
	{
		std::locale::global(std::locale(""));

		ResizeObj ro;
		ro.antialias(100);
		ro.headerRewriting(true);
		ro.specialColorMode(scmTOPLEFT);
		ro.addonPrefix("");
		ro.keepImageSize(false);
		ro.newExtension(".64.pak");
		ro.tileNoAnimation(false);

		std::vector<std::string> files;
		for(int i=1; i<argc; i++)
		{
			std::string key, val;
			parseArg(argv[i], key, val);
			if(key==""){
				if (val.find_first_of("*?")==std::string::npos)
					files.push_back(val);
				else
					fileList(files, val);
			}
			else if(key=="K"){ ro.keepImageSize(true); }
			else if(key=="KA"){ ro.tileNoAnimation(true); }
			else if(key=="A"){ ro.antialias(optToEnum<int>(val, 100, "A"));}
			else if(key=="S"){ ro.specialColorMode(optToEnum<SCConvMode>(val, 2, "S")); }
			else if(key=="W"){
				int ts = optToEnum<int>(val, 255, "W");
				if(ts%8 != 0) throw std::runtime_error("タイルサイズは8の倍数に限ります。"); 
				ro.newTileSize(ts);
			}
			else if(key=="N"){ ro.headerRewriting(false);    }
			else if(key=="T"){ ro.addonPrefix(val);        }
			else if(key=="E"){ if(val!="") ro.newExtension(val); }
			else if(key=="D"){ isShowErrorDialog = false; }
			else if(key=="H" || key=="?")
			{
				printOption();
				return 0;
			}
			else{
				std::clog << "無効なオプションです : " << key << std::endl;
				printOption();
				return 1;
			}
		}
		if(files.empty())
		{
			printOption();
			return 0;
		}

		for(std::vector<std::string>::iterator it = files.begin(); it != files.end(); it++)
		{
			if(*it == "con")
				ro.convertStdIO();
			else
				ro.convertFile(*it);
		}

	}catch(const std::exception &e)
	{
		std::cerr << "Error : " << e.what() << std::endl;
		if(isShowErrorDialog)
			showErrorDialog(e.what(), "Error");
		return 1;
	}		
	return 0;
}

