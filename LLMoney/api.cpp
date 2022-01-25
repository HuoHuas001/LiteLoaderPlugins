#include "pch.h"
#include "llmoney.h"
#include <memory>
#include <vector>
#include "Event.h"
#include <time.h>
#include <LoggerAPI.h>
#include <JsonLoader.h>
#include "../SDK/Header/third-party/Nlohmann/json.hpp"
using json = nlohmann::json;
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
	json j;			// 创建 json 对象
	std::ifstream jfile("plugins\\LLMoney\\mysql.json");
	if (jfile) {
		jfile >> j;		// 以文件流形式读取 json 文件
		host = (string)j["host"];
		user = (string)j["user"];
		pswd = (string)j["password"];
		table = (string)j["table"];
		port = (int)j["port"];
		return true;
	} 
	return false;
}

void disconnectdb() {
	mysql_free_result(res);
	mysql_close(con);
}

bool initDB() {
	getFile();
	try {
		mysql_library_init(0, NULL, NULL);//初始化MySQL库  
		con = mysql_init((MYSQL*)0);
		bool sqlConStatus = mysql_real_connect(con, host.c_str(), user.c_str(), pswd.c_str(), table.c_str(), port, NULL, 0);
		if (sqlConStatus) {
			moneylog.info("connect success.");
			const char* query = "set names \'UTF8\'";
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
				Time  INT NOT NULL , \
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
	//ConvertData();
	return true;
}

LLMONEY_API money_t LLMoneyGet(xuid_t xuid) {
	try {
		initDB();
		char queryC[400];
		sprintf(queryC, "SELECT Money FROM `money` WHERE XUID=%s", xuid.c_str());
		auto rt = mysql_real_query(con, queryC, strlen(queryC));
		if (rt)
		{
			moneylog.error("DB err {}", mysql_error(con));
			return -1;
		}

		res = mysql_store_result(con);//将结果保存在res结构体中
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
		char queryCmd[400];
		if (from != "") {
			//比对Money
			auto fromMoney = LLMoneyGet(from);
			if (fromMoney < val) {
				return false;
			}

			//数据上报
			fromMoney -= val;
			sprintf(queryCmd, "UPDATE `money` SET `Money`=%lld WHERE `XUID`='%s'", fromMoney, from.c_str());
			if (initDB()) {
				auto rt = mysql_query(con, queryCmd);
				if (rt) {
					moneylog.error("DB err {}", mysql_error(con));
				}
				else {
					res = mysql_store_result(con);
					mysql_free_result(res);
					mysql_close(con);
				}
			}
		}
		if (to != "") {
			auto toMoney = LLMoneyGet(to);
			if (toMoney < 0) {
				return false;
			}
			toMoney += val;
			sprintf(queryCmd, "UPDATE `money` SET `Money`=%lld WHERE `XUID`='%s'", toMoney, to.c_str());
			if (initDB()) {
				auto rt = mysql_query(con, queryCmd);
				if (rt) {
					moneylog.error("DB err {}", mysql_error(con));
				}
				else {
					res = mysql_store_result(con);
					mysql_free_result(res);
					mysql_close(con);
				}
			}
		}
		//上报Note
		initDB();
		char trans[400];
		time_t timestamp;
		sprintf(trans, "insert into mtrans (tFrom,tTo,Money,`Time`,Note) values ('%s','%s',%lld,%lld,'%s')", from.c_str(), to.c_str(), val, time(&timestamp), note.c_str());
		moneylog.info(trans);
		if (initDB()) {
			auto rt = mysql_query(con, trans);
			if (rt) {
				moneylog.error("DB err {}", mysql_error(con));
			}
			else {
				res = mysql_store_result(con);
				mysql_free_result(res);
				mysql_close(con);
			}
		}
		if (isRealTrans)
			CallAfterEvent(LLMoneyEvent::Trans, from, to, val);
		return true;
	}
	catch (std::exception const& e) {
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
		initDB();
		char query[400];
		std::ostringstream os;
		sprintf(query, "select tFrom,tTo,Money,datetime(Time,'unixepoch', 'localtime'),Note from mtrans where strftime('%s','now')-time<%i and (tFrom=%s OR tTo=%s) ORDER BY Time DESC","%s",timediff,xuid.c_str(), xuid.c_str());
		string rv;
		auto rt = mysql_real_query(con, query, strlen(query));
		if (rt)
		{
			moneylog.error("DB err {}", mysql_error(con));
			return "failed";
		}

		res = mysql_store_result(con);//将结果保存在res结构体中
		if (!res) {
			moneylog.error("DB err {}", mysql_error(con));
			return "failed";
		}
		while (auto row = mysql_fetch_row(res)) {
			optional<string> from, to;
			 from = PlayerInfo::fromXuid((string)row[0]);
			 to = PlayerInfo::fromXuid((string)row[1]);
			if (from.Set() && to.Set())
				if (from.val() == "") {
					from.val() = "System";
				}
				else if (to.val() == "") {
					to.val() = "System";
				}
				rv += from.val() + " -> " + to.val() + " " + std::to_string((money_t)atoll(row[2])) + " " + row[3] + " (" + row[4] + ")\n";
		}
		mysql_free_result(res);
		mysql_close(con);
		return rv;
	}
	catch (std::exception const& e) {
		moneylog.error("DB err {}\n", e.what());
		mysql_free_result(res);
		mysql_close(con);
		return "failed";
	}
}

LLMONEY_API void LLMoneyClearHist(int difftime) {
	try {
		initDB();
		char clear[400];
		sprintf(clear, "DELETE FROM mtrans WHERE strftime('%s','now')-time>%i","%s",difftime);
		mysql_query(con,clear);
		mysql_free_result(res);
		mysql_close(con);
	}
	catch (std::exception&) {

	}
}

void ConvertData(){};
//void ConvertData() {
//	if (std::filesystem::exists("plugins\\LLMoney\\money.db")) {
//		moneylog.info("Old money data detected, try to convert old data to new data");
//		try {
//			std::unique_ptr<SQLite::Database> db2 = std::make_unique<SQLite::Database>("plugins\\LLMoney\\money.db", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
//			SQLite::Statement get{ *db2, "select hex(XUID),Money from money" };
//			SQLite::Statement set{ *db,"insert into money values (?,?)" };
//			while (get.executeStep()) {
//				std::string blob = get.getColumn(0).getText();
//				unsigned long long value;
//				std::istringstream iss(blob);
//				iss >> std::hex >> value;
//				unsigned long long xuid =_byteswap_uint64(value);
//				long long money = get.getColumn(1).getInt64();
//				set.bindNoCopy(1, std::to_string(xuid));
//				set.bind(2, money);
//				set.exec();
//				set.reset();
//				set.clearBindings();
//			}
//			get.reset();
//		}
//		catch (std::exception& e) {
//			moneylog.error("{}", e.what());
//		}
//		std::filesystem::rename("plugins\\LLMoney\\money.db", "plugins\\LLMoney\\money_old.db");
//		moneylog.info("Conversion completed");
//	}
//}

