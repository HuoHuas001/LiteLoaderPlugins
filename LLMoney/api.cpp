#include "pch.h"
#include "llmoney.h"
#include <memory>
#include <vector>
#include "Event.h"
#include <LoggerAPI.h>
#include <JsonLoader.h>
#include "../SDK/Header/third-party/Nlohmann/json.hpp"
using json = nlohmann::json;
static std::unique_ptr<SQLite::Database> db;
Logger moneylog("LLMoney");
money_t DEF_MONEY = 0;

//MySQL
MYSQL* con;
MYSQL_RES*res;
MYSQL_ROW row;
//database configuartion
string user = "root";
string pswd = "123456"; // it must be    changed
string host = "localhost";
string table = "hospital";
unsigned port = 3306;

struct cleanSTMT {
	SQLite::Statement& get;
	cleanSTMT(SQLite::Statement& g) :get(g) {

	}
	~cleanSTMT() {
		get.reset();
		get.clearBindings();
	}
};

void ConvertData();
bool getFile() {
	json j;			// ���� json ����
	std::ifstream jfile("plugins\\LLMoney\\mysql.json");
	if (jfile) {
		jfile >> j;		// ���ļ�����ʽ��ȡ json �ļ�
		host = (string)j["host"];
		user = (string)j["user"];
		pswd = (string)j["password"];
		table = (string)j["table"];
		port = (int)j["port"];
		return true;
	} 
	return false;
}

void disconnect() {
	mysql_free_result(res);
	mysql_close(con);
}

bool initDB() {
	getFile();
	try {
		mysql_library_init(0, NULL, NULL);//��ʼ��MySQL��  
		con = mysql_init((MYSQL*)0);
		bool sqlConStatus = mysql_real_connect(con, host.c_str(), user.c_str(), pswd.c_str(), table.c_str(), port, NULL, 0);
		if (sqlConStatus) {
			const char* query = "set names \'GBK\'";
			mysql_real_query(con, query, strlen(query));
			mysql_query(con, "PRAGMA journal_mode = MEMORY");
			mysql_query(con,"PRAGMA synchronous = NORMAL");
			mysql_query(con, ("CREATE TABLE IF NOT EXISTS money(\
				XUID TEXT NOT NULL,\
				Money INT NOT NULL\
				); "));
			mysql_query(con, "CREATE TABLE IF NOT EXISTS mtrans ( \
				tFrom TEXT  NOT NULL, \
				tTo   TEXT  NOT NULL, \
				Money INT  NOT NULL, \
				Time  NUMERIC NOT NULL , \
				Note  TEXT \
			);");
			mysql_query(con, "CREATE INDEX IF NOT EXISTS idx ON mtrans ( \
				Time COLLATE BINARY COLLATE BINARY DESC \
			); ");
		}
		else {
			moneylog.error("DB err {}", mysql_error(con));
			return false;
		}
		
	}
	catch (std::exception const& e) {
		moneylog.error("DB err {}", e.what());
		return false;
	}
	ConvertData();
	return true;
}

LLMONEY_API money_t LLMoneyGet(xuid_t xuid) {
	try {
		initDB();
		char queryC[400];
		sprintf(queryC, "SELECT Money FROM `money` WHERE XUID=%s", xuid.c_str());
		moneylog.info(queryC);
		auto rt = mysql_real_query(con, queryC, strlen(queryC));
		if (rt)
		{
			moneylog.error("DB err {}", mysql_error(con));
			return -1;
		}

		res = mysql_store_result(con);//�����������res�ṹ����
		if (!res) {
			moneylog.error("DB err {}", mysql_error(con));
			return -1;
		}
		money_t rv = DEF_MONEY;
		bool fg = false;
		while (auto row = mysql_fetch_row(res)) {
			rv = (money_t)atoll(row[0]);
			fg = true;
		}

		if (!fg) {
			std::ostringstream os;
			char queryCs[400];
			sprintf(queryCs, "INSERT INTO `money`(`XUID`, `Money`) VALUES (%s,%lld)",xuid.c_str(),DEF_MONEY);
			mysql_query(con, queryCs);
			rv = DEF_MONEY;
		}
		mysql_free_result(res);
		mysql_close(con);
		return rv;
	}
	catch (std::exception const& e) {
		moneylog.error("DB err {}\n", e.what());
		return -1;
	}
	
}

bool isRealTrans = true;

LLMONEY_API bool LLMoneyTrans(xuid_t from, xuid_t to, money_t val, string const& note)
{
	bool isRealTrans = ::isRealTrans;
	::isRealTrans = true;
	if(isRealTrans)
		if (!CallBeforeEvent(LLMoneyEvent::Trans, from, to, val))
			return false;

	if (val < 0 || from == to)
		return false;
	try {
		char updateC[400];
		sprintf(updateC, "UPDATE `money` SET `Money`=%lld WHERE `XUID`=%s");
		db->exec("begin");
		static SQLite::Statement set{ *db,"update money set Money=? where XUID=?" };
		if (from != "") {
			auto fmoney = LLMoneyGet(from);
			if (fmoney < val) {
				db->exec("rollback");
				return false;
			}
			fmoney -= val;
			{

				set.bindNoCopy(2, from);
				set.bind(1, fmoney);
				set.exec();
				set.reset();
				set.clearBindings();
			}
		}
		if (to != "") {
			auto tmoney = LLMoneyGet(to);
			tmoney += val;
			if (tmoney < 0) {
				db->exec("rollback");
				return false;
			}
			{
				set.bindNoCopy(2, to);
				set.bind(1, tmoney);
				set.exec();
				set.reset();
				set.clearBindings();
			}
		}
		{
			static SQLite::Statement addTrans{ *db,"insert into mtrans (tFrom,tTo,Money,Note) values (?,?,?,?)" };
			addTrans.bindNoCopy(1, from);
			addTrans.bindNoCopy(2, to);
			addTrans.bind(3, val);
			addTrans.bindNoCopy(4, note);
			addTrans.exec();
			addTrans.reset();
			addTrans.clearBindings();
		}
		db->exec("commit");

		if (isRealTrans)
			CallAfterEvent(LLMoneyEvent::Trans, from, to, val);
		return true;
	}
	catch (std::exception const& e) {
		db->exec("rollback");
		moneylog.error("DB err {}\n", e.what());
		return false;
	}
}

LLMONEY_API bool LLMoneyAdd(xuid_t xuid, money_t money)
{
	if (!CallBeforeEvent(LLMoneyEvent::Add, "", xuid, money))
		return false;

	isRealTrans = false;
	bool res = LLMoneyTrans("", xuid, money, "add " + std::to_string(money));
	if(res)
		CallAfterEvent(LLMoneyEvent::Add, "", xuid, money);
	return res;
}

LLMONEY_API bool LLMoneyReduce(xuid_t xuid, money_t money)
{
	if (!CallBeforeEvent(LLMoneyEvent::Reduce, "", xuid, money))
		return false;

	isRealTrans = false;
	bool res = LLMoneyTrans(xuid, "", money, "reduce " + std::to_string(money));
	if (res)
		CallAfterEvent(LLMoneyEvent::Reduce, "", xuid, money);
	return res;
}

LLMONEY_API bool LLMoneySet(xuid_t xuid, money_t money)
{
	if (!CallBeforeEvent(LLMoneyEvent::Set, "", xuid, money))
		return false;
	money_t now = LLMoneyGet(xuid), diff;
	xuid_t from, to;
	if (money >= now) {
		from = "";
		to = xuid;
		diff = money - now;
	}
	else {
		from = xuid;
		to = "";
		diff = now - money;
	}

	isRealTrans = false;
	bool res = LLMoneyTrans(from, to, diff, "set to " + std::to_string(money));
	if (res)
		CallAfterEvent(LLMoneyEvent::Reduce, "", xuid, money);
	return res;
}


LLMONEY_API string LLMoneyGetHist(xuid_t xuid, int timediff)
{
	try {
		static SQLite::Statement get{ *db,"select tFrom,tTo,Money,datetime(Time,'unixepoch', 'localtime'),Note from mtrans where strftime('%s','now')-time<? and (tFrom=? OR tTo=?) ORDER BY Time DESC" };
		string rv;
		get.bind(1, timediff);
		get.bindNoCopy(2, xuid);
		get.bindNoCopy(3, xuid);
		while (get.executeStep()) {
			optional<string> from, to;
			 from = PlayerInfo::fromXuid(get.getColumn(0).getString());
			 to = PlayerInfo::fromXuid(get.getColumn(1).getString());
			if (from.Set() && to.Set())
				if (from.val() == "") {
					from.val() = "System";
				}
				else if (to.val() == "") {
					to.val() = "System";
				}
				rv += from.val() + " -> " + to.val() + " " + std::to_string((money_t)get.getColumn(2).getInt64()) + " " + get.getColumn(3).getText() + " (" + get.getColumn(4).getText() + ")\n";
		}
		get.reset();
		get.clearBindings();
		return rv;
	}
	catch (std::exception const& e) {
		moneylog.error("DB err {}\n", e.what());
		return "failed";
	}
}

LLMONEY_API void LLMoneyClearHist(int difftime) {
	try {
		db->exec("DELETE FROM mtrans WHERE strftime('%s','now')-time>" + std::to_string(difftime));
	}
	catch (std::exception&) {

	}
}

void ConvertData() {
	if (std::filesystem::exists("plugins\\LLMoney\\money.db")) {
		moneylog.info("Old money data detected, try to convert old data to new data");
		try {
			std::unique_ptr<SQLite::Database> db2 = std::make_unique<SQLite::Database>("plugins\\LLMoney\\money.db", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
			SQLite::Statement get{ *db2, "select hex(XUID),Money from money" };
			SQLite::Statement set{ *db,"insert into money values (?,?)" };
			while (get.executeStep()) {
				std::string blob = get.getColumn(0).getText();
				unsigned long long value;
				std::istringstream iss(blob);
				iss >> std::hex >> value;
				unsigned long long xuid =_byteswap_uint64(value);
				long long money = get.getColumn(1).getInt64();
				set.bindNoCopy(1, std::to_string(xuid));
				set.bind(2, money);
				set.exec();
				set.reset();
				set.clearBindings();
			}
			get.reset();
		}
		catch (std::exception& e) {
			moneylog.error("{}", e.what());
		}
		std::filesystem::rename("plugins\\LLMoney\\money.db", "plugins\\LLMoney\\money_old.db");
		moneylog.info("Conversion completed");
	}
}

