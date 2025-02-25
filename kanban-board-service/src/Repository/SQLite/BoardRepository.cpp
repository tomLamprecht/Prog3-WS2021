#include "BoardRepository.hpp"
#include "Core/Exception/NotImplementedException.hpp"
#include <ctime>
#include <filesystem>
#include <iterator>
#include <sstream>
#include <string.h>
#include <string>
#include <tr1/unordered_map>
#include <vector>

using namespace Prog3::Repository::SQLite;
using namespace Prog3::Core::Model;
using namespace Prog3::Core::Exception;
using namespace std;

#ifdef RELEASE_SERVICE
string const BoardRepository::databaseFile = "./data/kanban-board.db";
#else
string const BoardRepository::databaseFile = "../data/kanban-board.db";
#endif

BoardRepository::BoardRepository() : database(nullptr) {

    string databaseDirectory = filesystem::path(databaseFile).parent_path().string();

    if (filesystem::is_directory(databaseDirectory) == false) {
        filesystem::create_directory(databaseDirectory);
    }

    int result = sqlite3_open(databaseFile.c_str(), &database);

    if (SQLITE_OK != result) {
        cout << "Cannot open database: " << sqlite3_errmsg(database) << endl;
    }

    initialize();
}

BoardRepository::~BoardRepository() {
    sqlite3_close(database);
}

void BoardRepository::initialize() {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlCreateTableColumn =
        "create table if not exists column("
        "id integer not null primary key autoincrement,"
        "name text not null,"
        "position integer not null UNIQUE);";

    string sqlCreateTableItem =
        "create table if not exists item("
        "id integer not null primary key autoincrement,"
        "title text not null,"
        "date text not null,"
        "position integer not null,"
        "column_id integer not null,"
        "unique (position, column_id),"
        "foreign key (column_id) references column (id));";

    result = sqlite3_exec(database, sqlCreateTableColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
    result = sqlite3_exec(database, sqlCreateTableItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    // only if dummy data is needed ;)
    createDummyData();
}

Board BoardRepository::getBoard() {
    Board board("Kanban Board");
    board.setColumns(getColumns());
    return board;
}

std::vector<Column> BoardRepository::getColumns() {
    string itemSqlSelect = "SELECT id FROM column;";
    char *errorMessage = nullptr;
    vector<int> ids;
    vector<int> *idsP = &ids;
    int answer = sqlite3_exec(database, itemSqlSelect.c_str(), BoardRepository::getIdCallback, idsP, &errorMessage);
    handleSQLError(answer, errorMessage);

    if (answer != SQLITE_OK) {
        vector<Column> emptyVector;
        return emptyVector;
    }
    vector<Column> columns;

    for (auto id : ids) {
        optional<Column> temp = getColumn(id);
        if (temp.has_value()) {
            columns.push_back(temp.value());
        }
    }
    return columns;
}

std::optional<Column> BoardRepository::getColumn(int id) {
    string sqlSelect = "SELECT * FROM column WHERE id=" + to_string(id) + ";";
    char *errorMessage = nullptr;
    //this is done to reserve the needed memory
    Column column(-1, "", 0);
    Column *columnP = &column;
    vector<Item> tempItems = getItems(id);

    int result = sqlite3_exec(database, sqlSelect.c_str(), BoardRepository::getColumnCallback, columnP, &errorMessage);
    handleSQLError(result, errorMessage);

    for (auto item : tempItems) {
        column.addItem(item);
    }

    if (result != SQLITE_OK || column.getId() == -1)
        return nullopt;

    return column;
}

std::optional<Column> BoardRepository::postColumn(std::string name, int position) {
    string sqlPostItem =
        "INSERT INTO column('name', 'position') "
        "VALUES('" +
        name + "', '" + to_string(position) + "')";

    int result = 0;
    char *errorMessage = nullptr;

    result = sqlite3_exec(database, sqlPostItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK == result) {
        int columnId = sqlite3_last_insert_rowid(database);
        return Column(columnId, name, position);
    }

    return std::nullopt;
}

template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

std::optional<Prog3::Core::Model::Column> BoardRepository::putColumn(int id, std::string name, int position) {
    int result = 0;
    char *errorMessage = nullptr;
    string emptyString = "";
    string emptyStringThisColumn = "";
    void *selectResult = static_cast<void *>(&emptyString);
    void *thisColumn = static_cast<void *>(&emptyStringThisColumn);

    string sqlSelectItems = "SELECT * from item WHERE column_id=" + to_string(id) + ";";
    string sqlPutColumn = "UPDATE column SET name= \"" + name + "\", position = " + to_string(position) + " WHERE id = " + to_string(id) + ";";
    string sqlSelectColumn = "SELECT * from column WHERE id=" + to_string(id) + ";";

    //CHECK IF COLUMN EXISTS
    result = sqlite3_exec(database, sqlSelectColumn.c_str(), BoardRepository::queryCallback, thisColumn, &errorMessage);
    handleSQLError(result, errorMessage);
    string *tempPointer = static_cast<std::string *>(thisColumn);
    string thisColumnString = *tempPointer;
    if (thisColumnString == "") {
        return std::nullopt;
    }

    //GET ALL ITEMS OUT OF THE COLUMN
    result = sqlite3_exec(database, sqlSelectItems.c_str(), BoardRepository::queryCallback, selectResult, &errorMessage);
    handleSQLError(result, errorMessage);
    string *sp = static_cast<std::string *>(selectResult);
    string data = *sp;
    std::vector items = split(data, ';');
    vector<Item> realItems{};
    for (auto item : items) {
        vector tuples = split(item, ',');

        string itemIdString = split(tuples[0], ':')[1];
        int itemId = std::stoi(itemIdString);

        string title = split(tuples[1], ':')[1];

        string datetime = split(tuples[2], ':')[1];

        string positionString = split(tuples[3], ':')[1];
        int position = std::stoi(positionString);

        //cout << "Id: " + to_string(itemId) + " title: " + title + " position: " + to_string(position) + " datetime: " + datetime << endl;
        realItems.push_back(Item(itemId, title, position, datetime));
    }

    //UPDATE COLUMN
    errorMessage = nullptr;
    result = sqlite3_exec(database, sqlPutColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK != result) {
        std::cout << "Could not edit column with id " + to_string(id) + "." << endl;
        return std::nullopt;
    }
    std::cout << "Column with id " + to_string(id) + " sucessfully edited" << endl;
    Column column(id, name, position);
    for (auto item : realItems) {
        column.addItem(item);
    }
    return column;
}

void BoardRepository::deleteColumn(int id) {
    string sqlDeleteColumn = "DELETE FROM column WHERE id = " + to_string(id) + ";";

    int result = 0;
    char *errorMessage = nullptr;

    result = sqlite3_exec(database, sqlDeleteColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK != result) {
        std::cout << "Could not delete column with id " + to_string(id) + "." << endl;
    } else {
        std::cout << "Column with id " + to_string(id) + " sucessfully deleted" << endl;
    }
}

std::vector<Item> BoardRepository::getItems(int columnId) {
    string itemSqlSelect = "SELECT * FROM item WHERE column_id=" + to_string(columnId) + ";";
    char *errorMessage = nullptr;
    vector<Item> tempItems;
    vector<Item> *items = &tempItems;
    int answer = sqlite3_exec(database, itemSqlSelect.c_str(), BoardRepository::getItemCallback, items, &errorMessage);
    handleSQLError(answer, errorMessage);

    if (answer != SQLITE_OK) {
        vector<Item> emptyVector;
        return emptyVector;
    }
    return tempItems;
}

std::optional<Item> BoardRepository::getItem(int columnId, int itemId) {
    string itemSqlSelect = "SELECT * FROM item WHERE column_id=" + to_string(columnId) + " AND id=" + to_string(itemId) + ";";
    char *errorMessage = nullptr;
    vector<Item> tempItems;
    vector<Item> *items = &tempItems;

    int answer = sqlite3_exec(database, itemSqlSelect.c_str(), BoardRepository::getItemCallback, items, &errorMessage);
    handleSQLError(answer, errorMessage);

    if (answer != SQLITE_OK || tempItems.size() == 0) {
        return nullopt;
    }
    return tempItems[0];
}

std::optional<Item> BoardRepository::postItem(int columnId, std::string title, int position) {
    time_t now = time(0);
    char *datetime = ctime(&now);

    string sqlPostItem =
        "INSERT INTO item ('title', 'date', 'position', 'column_id') "
        "VALUES ('" +
        title + "', '" + datetime + "', " + to_string(position) + ", " + to_string(columnId) + ");";

    int result = 0;
    char *errorMessage = nullptr;

    result = sqlite3_exec(database, sqlPostItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    int itemId = INVALID_ID;
    if (SQLITE_OK == result) {
        itemId = sqlite3_last_insert_rowid(database);
        return Item(itemId, title, position, datetime);
    }
    return std::nullopt;
}

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    return 1;
}

std::optional<Prog3::Core::Model::Item> BoardRepository::putItem(int columnId, int itemId, std::string title, int position) {
    time_t now = time(0);
    char *datetime = ctime(&now);

    char *errorMessage = nullptr;
    int result = 0;

    // COMMENT THIS IS IN IF YOU WANT TO PREVENT USERS CREATING RESOURCES VIA PUT INSTEAD OF POST
    void *selectResult = static_cast<void *>(new std::string(""));
    string sqlSelect = "SELECT * FROM item WHERE id =" + to_string(itemId) + ";";
    int selectAnswer = sqlite3_exec(database, sqlSelect.c_str(), BoardRepository::queryCallback, selectResult, &errorMessage);
    handleSQLError(selectAnswer, errorMessage);

    string *tempPointer = static_cast<std::string *>(selectResult);
    string thisItemString = *tempPointer;
    if (thisItemString == "") {
        return std::nullopt;
    }

    string sqlPutItem = "UPDATE item SET title =\"" + title + "\"" + ", position =" + to_string(position) + ", column_id= " + to_string(columnId) + " WHERE id = " + to_string(itemId) + ";";

    result = sqlite3_exec(database, sqlPutItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK != result) {
        std::cout << "Could not edit item with id " + to_string(itemId) + "." << endl;
        return std::nullopt;
    }
    std::cout << "Item with id " + to_string(itemId) + " sucessfully edited" << endl;
    return Item(itemId, title, position, datetime);
}

void BoardRepository::deleteItem(int columnId, int itemId) {

    string sqlDeleteColumn = "DELETE FROM item WHERE id = " + to_string(itemId) + " AND column_id = " + to_string(columnId) + ";";

    int result = 0;
    char *errorMessage = nullptr;

    result = sqlite3_exec(database, sqlDeleteColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK != result) {
        std::cout << "Could not delete Item with id " + to_string(itemId) + " in Column + " + to_string(columnId) + "." << endl;
    } else {
        std::cout << "Item with id " + to_string(itemId) + " in Column + " + to_string(columnId) + " sucessfully deleted." << endl;
    }
}

void BoardRepository::handleSQLError(int statementResult, char *errorMessage) {

    if (statementResult != SQLITE_OK) {
        cout << "SQL error: " << errorMessage << endl;
        sqlite3_free(errorMessage);
    }
}

void BoardRepository::createDummyData() {

    cout << "creatingDummyData ..." << endl;

    int result = 0;
    char *errorMessage;
    string sqlInsertDummyColumns =
        "insert into column (name, position)"
        "VALUES"
        "(\"prepare\", 1),"
        "(\"running\", 2),"
        "(\"finished\", 3);";

    result = sqlite3_exec(database, sqlInsertDummyColumns.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    string sqlInserDummyItems =
        "insert into item (title, date, position, column_id)"
        "VALUES"
        "(\"in plan\", date('now'), 1, 1),"
        "(\"some running task\", date('now'), 1, 2),"
        "(\"finished task 1\", date('now'), 1, 3),"
        "(\"finished task 2\", date('now'), 2, 3);";

    result = sqlite3_exec(database, sqlInserDummyItems.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

/*
  I know source code comments are bad, but this one is to guide you through the use of sqlite3_exec() in case you want to use it.
  sqlite3_exec takes a "Callback function" as one of its arguments, and since there are many crazy approaches in the wild internet,
  I want to show you how the signature of this "callback function" may look like in order to work with sqlite3_exec()
*/

//The data is in following format: Every Column is differenced by a ";" every entry is differenced by a "," and every Key Value Pair is Differenced by a ":"
int BoardRepository::queryCallback(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    string *stringPointer = static_cast<string *>(data);
    for (int i = 0; i < numberOfColumns; i++) {
        *stringPointer = *stringPointer + columnNames[i] + ":" + fieldValues[i];
        if (i < numberOfColumns - 1)
            *stringPointer = *stringPointer + ",";
    }
    *stringPointer = *stringPointer + ";";
    return 0;
}

// id:5,title:test,position:100;id:6,title:test2,postion101

int BoardRepository::getColumnCallback(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    Column *column = static_cast<Column *>(data);
    vector<std::string> values;
    for (int i = 0; i < numberOfColumns; i++) {
        values.push_back(*(fieldValues++));
    }
    int id = std::stoi(values[0]);
    string title = values[1];
    int position = std::stoi(values[2]);

    column->setID(id);
    column->setName(title);
    column->setPos(position);
    return 0;
}

int BoardRepository::getItemCallback(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    vector<Item> *items = static_cast<vector<Item> *>(data);
    vector<std::string> values;
    for (int i = 0; i < numberOfColumns; i++) {
        values.push_back(*(fieldValues++));
    }
    int id = std::stoi(values[0]);
    string title = values[1];
    string date = values[2];
    int position = std::stoi(values[3]);
    Item temp(id, title, position, date);
    items->push_back(temp);
    return 0;
}

int BoardRepository::getIdCallback(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    if (numberOfColumns == 0) {
        return 0;
    }
    vector<int> *ids = static_cast<vector<int> *>(data);
    ids->push_back(stoi(fieldValues[0]));
    return 0;
}
