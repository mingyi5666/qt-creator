/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "googletest.h"

#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitereadwritestatement.h>
#include <sqlitewritestatement.h>

#include <utils/smallstringio.h>

#include <QDir>

#include <vector>

namespace {

using Sqlite::JournalMode;
using Sqlite::Exception;
using Sqlite::Database;
using Sqlite::ReadStatement;
using Sqlite::ReadWriteStatement;
using Sqlite::WriteStatement;

MATCHER_P3(HasValues, value1, value2, rowid,
           std::string(negation ? "isn't" : "is")
           + PrintToString(value1)
           + ", " + PrintToString(value2)
           + " and " + PrintToString(rowid)
           )
{
    Database &database = arg.database();

    ReadStatement statement("SELECT name, number FROM test WHERE rowid=?", database);
    statement.bind(1, rowid);

    statement.next();

    return statement.text(0) == value1 && statement.text(1) == value2;
}

class SqliteStatement : public ::testing::Test
{
protected:
     void SetUp() override;
     void TearDown() override;

protected:
    Database database;
};

struct Output
{
    Utils::SmallString name;
    Utils::SmallString number;
    long long value;
    friend bool operator==(const Output &f, const Output &s)
    {
        return f.name == s.name && f.number == s.number && f.value == s.value;
    }
    friend std::ostream &operator<<(std::ostream &out, const Output &o)
    {
        return out << "(" << o.name << ", " << ", " << o.number<< ", " << o.value<< ")";
    }
};

TEST_F(SqliteStatement, PrepareFailure)
{
    ASSERT_THROW(ReadStatement("blah blah blah", database), Exception);
    ASSERT_THROW(WriteStatement("blah blah blah", database), Exception);
    ASSERT_THROW(ReadStatement("INSERT INTO test(name, number) VALUES (?, ?)", database), Exception);
    ASSERT_THROW(WriteStatement("SELECT name, number FROM test '", database), Exception);
}

TEST_F(SqliteStatement, CountRows)
{
    ReadStatement statement("SELECT * FROM test", database);
    int nextCount = 0;
    while (statement.next())
        ++nextCount;

    int sqlCount = ReadStatement::toValue<int>("SELECT count(*) FROM test", database);

    ASSERT_THAT(nextCount, sqlCount);
}

TEST_F(SqliteStatement, Value)
{
    ReadStatement statement("SELECT name, number FROM test ORDER BY name", database);
    statement.next();

    statement.next();

    ASSERT_THAT(statement.value<int>(0), 0);
    ASSERT_THAT(statement.value<int64_t>(0), 0);
    ASSERT_THAT(statement.value<double>(0), 0.0);
    ASSERT_THAT(statement.text(0), "foo");
    ASSERT_THAT(statement.value<int>(1), 23);
    ASSERT_THAT(statement.value<int64_t>(1), 23);
    ASSERT_THAT(statement.value<double>(1), 23.3);
    ASSERT_THAT(statement.text(1), "23.3");
}

TEST_F(SqliteStatement, ValueFailure)
{
    ReadStatement statement("SELECT name, number FROM test", database);
    ASSERT_THROW(statement.value<int>(0), Exception);

    statement.reset();

    while (statement.next()) {}
    ASSERT_THROW(statement.value<int>(0), Exception);

    statement.reset();

    statement.next();
    ASSERT_THROW(statement.value<int>(-1), Exception);
    ASSERT_THROW(statement.value<int>(2), Exception);
}

TEST_F(SqliteStatement, ToIntergerValue)
{
    auto value = ReadStatement::toValue<int>("SELECT number FROM test WHERE name='foo'", database);

    ASSERT_THAT(value, 23);
}

TEST_F(SqliteStatement, ToLongIntergerValue)
{
    ASSERT_THAT(ReadStatement::toValue<qint64>("SELECT number FROM test WHERE name='foo'", database), Eq(23));
}

TEST_F(SqliteStatement, ToDoubleValue)
{
    ASSERT_THAT(ReadStatement::toValue<double>("SELECT number FROM test WHERE name='foo'", database), 23.3);
}

TEST_F(SqliteStatement, ToStringValue)
{
    ASSERT_THAT(ReadStatement::toValue<Utils::SmallString>("SELECT name FROM test WHERE name='foo'", database), "foo");
}

TEST_F(SqliteStatement, ColumnNames)
{
    ReadStatement statement("SELECT name, number FROM test", database);

    auto columnNames = statement.columnNames();

    ASSERT_THAT(columnNames, ElementsAre("name", "number"));
}

TEST_F(SqliteStatement, BindString)
{

    ReadStatement statement("SELECT name, number FROM test WHERE name=?", database);

    statement.bind(1, "foo");

    statement.next();

    ASSERT_THAT(statement.text(0), "foo");
    ASSERT_THAT(statement.value<double>(1), 23.3);
}

TEST_F(SqliteStatement, BindInteger)
{
    ReadStatement statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 40);
    statement.next();

    ASSERT_THAT(statement.text(0),"poo");
}

TEST_F(SqliteStatement, BindLongInteger)
{
    ReadStatement statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, int64_t(40));
    statement.next();

    ASSERT_THAT(statement.text(0), "poo");
}

TEST_F(SqliteStatement, BindDouble)
{
    ReadStatement statement("SELECT name, number FROM test WHERE number=?", database);

    statement.bind(1, 23.3);
    statement.next();

    ASSERT_THAT(statement.text(0), "foo");
}

TEST_F(SqliteStatement, BindIntegerByParameter)
{
    ReadStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    statement.bind("@number", 40);
    statement.next();

    ASSERT_THAT(statement.text(0), "poo");
}

TEST_F(SqliteStatement, BindLongIntegerByParameter)
{
    ReadStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    statement.bind("@number", int64_t(40));
    statement.next();

    ASSERT_THAT(statement.text(0), "poo");
}

TEST_F(SqliteStatement, BindDoubleByIndex)
{
    ReadStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    statement.bind(statement.bindingIndexForName("@number"), 23.3);
    statement.next();

    ASSERT_THAT(statement.text(0), "foo");
}

TEST_F(SqliteStatement, BindFailure)
{
    ReadStatement statement("SELECT name, number FROM test WHERE number=@number", database);

    ASSERT_THROW(statement.bind(0, 40), Exception);
    ASSERT_THROW(statement.bind(2, 40), Exception);
    ASSERT_THROW(statement.bind("@name", 40), Exception);
}

TEST_F(SqliteStatement, RequestBindingNamesFromStatement)
{
    WriteStatement statement("UPDATE test SET name=@name, number=@number WHERE rowid=@id", database);

    ASSERT_THAT(statement.bindingColumnNames(), ElementsAre("name", "number", "id"));
}

TEST_F(SqliteStatement, BindValues)
{
    WriteStatement statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.bindValues("see", 7.23, 1);
    statement.execute();

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, WriteValues)
{
    WriteStatement statement("UPDATE test SET name=?, number=? WHERE rowid=?", database);

    statement.write("see", 7.23, 1);

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, BindNamedValues)
{
    WriteStatement statement("UPDATE test SET name=@name, number=@number WHERE rowid=@id", database);

    statement.bindNameValues("@name", "see", "@number", 7.23, "@id", 1);
    statement.execute();

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, WriteNamedValues)
{
    WriteStatement statement("UPDATE test SET name=@name, number=@number WHERE rowid=@id", database);

    statement.writeNamed("@name", "see", "@number", 7.23, "@id", 1);

    ASSERT_THAT(statement, HasValues("see", "7.23", 1));
}

TEST_F(SqliteStatement, ClosedDatabase)
{
    database.close();
    ASSERT_THROW(WriteStatement("INSERT INTO test(name, number) VALUES (?, ?)", database), Exception);
    ASSERT_THROW(ReadStatement("SELECT * FROM test", database), Exception);
    ASSERT_THROW(ReadWriteStatement("INSERT INTO test(name, number) VALUES (?, ?)", database), Exception);
    database.open(QDir::tempPath() + QStringLiteral("/SqliteStatementTest.db"));
}

TEST_F(SqliteStatement, GetTupleValuesWithoutArguments)
{
    using Tuple = std::tuple<Utils::SmallString, double, int>;
    ReadStatement statement("SELECT name, number, value FROM test", database);

    auto values = statement.tupleValues<Utils::SmallString, double, int>(3);

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", 0, 1},
                                    Tuple{"foo", 23.3, 2},
                                    Tuple{"poo", 40.0, 3}));
}

TEST_F(SqliteStatement, GetSingleValuesWithoutArguments)
{
    ReadStatement statement("SELECT name FROM test", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3);

    ASSERT_THAT(values, ElementsAre("bar", "foo", "poo"));
}

TEST_F(SqliteStatement, GetStructValuesWithoutArguments)
{
    ReadStatement statement("SELECT name, number, value FROM test", database);

    auto values = statement.structValues<Output, Utils::SmallString, Utils::SmallString, long long>(3);

    ASSERT_THAT(values, ElementsAre(Output{"bar", "blah", 1},
                                    Output{"foo", "23.3", 2},
                                    Output{"poo", "40", 3}));
}

TEST_F(SqliteStatement, GetValuesForSingleOutputWithBindingMultipleTimes)
{
    ReadStatement statement("SELECT name FROM test WHERE number=?", database);
    statement.values<Utils::SmallString>(3, 40);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3, 40);

    ASSERT_THAT(values, ElementsAre("poo"));
}

TEST_F(SqliteStatement, GetValuesForMultipleOutputValuesAndContainerQueryValues)
{
    using Tuple = std::tuple<Utils::SmallString, double, double>;
    std::vector<double> queryValues = {40, 23.3};
    ReadStatement statement("SELECT name, number, value FROM test WHERE number=?", database);

    auto values = statement.tupleValues<Utils::SmallString, double, double>(3, queryValues);

    ASSERT_THAT(values, ElementsAre(Tuple{"poo", 40, 3.},
                                    Tuple{"foo", 23.3, 2.}));
}

TEST_F(SqliteStatement, GetValuesForSingleOutputValuesAndContainerQueryValues)
{
    std::vector<double> queryValues = {40, 23.3};
    ReadStatement statement("SELECT name, number FROM test WHERE number=?", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3, queryValues);

    ASSERT_THAT(values, ElementsAre("poo", "foo"));
}

TEST_F(SqliteStatement, GetValuesForMultipleOutputValuesAndContainerQueryTupleValues)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, int>;
    using Tuple2 = std::tuple<Utils::SmallString, double, int>;
    std::vector<Tuple> queryValues = {{"poo", "40", 3}, {"bar", "blah", 1}};
    ReadStatement statement("SELECT name, number, value FROM test WHERE name= ? AND number=? AND value=?", database);

    auto values = statement.tupleValues<Utils::SmallString, double, int>(3, queryValues);

    ASSERT_THAT(values, ElementsAre(Tuple2{"poo", 40, 3},
                                    Tuple2{"bar", 0, 1}));
}

TEST_F(SqliteStatement, GetValuesForSingleOutputValuesAndContainerQueryTupleValues)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString>;
    std::vector<Tuple> queryValues = {{"poo", "40"}, {"bar", "blah"}};
    ReadStatement statement("SELECT name, number FROM test WHERE name= ? AND number=?", database);

    std::vector<Utils::SmallString> values = statement.values<Utils::SmallString>(3, queryValues);

    ASSERT_THAT(values, ElementsAre("poo", "bar"));
}

TEST_F(SqliteStatement, GetValuesForMultipleOutputValuesAndMultipleQueryValue)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement statement("SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto values = statement.tupleValues<Utils::SmallString, Utils::SmallString, long long>(3, "bar", "blah", 1);

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, CallGetValuesForMultipleOutputValuesAndMultipleQueryValueMultipleTimes)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, long long>;
    ReadStatement statement("SELECT name, number, value FROM test WHERE name=? AND number=?", database);
    statement.tupleValues<Utils::SmallString, Utils::SmallString, long long>(3, "bar", "blah");

    auto values = statement.tupleValues<Utils::SmallString, Utils::SmallString, long long>(3, "bar", "blah");

    ASSERT_THAT(values, ElementsAre(Tuple{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetStructOutputValuesAndMultipleQueryValue)
{
    ReadStatement statement("SELECT name, number, value FROM test WHERE name=? AND number=? AND value=?", database);

    auto values = statement.structValues<Output, Utils::SmallString, Utils::SmallString, long long>(3, "bar", "blah", 1);

    ASSERT_THAT(values, ElementsAre(Output{"bar", "blah", 1}));
}

TEST_F(SqliteStatement, GetStructOutputValuesAndContainerQueryValues)
{
    std::vector<double> queryValues = {40, 23.3};
    ReadStatement statement("SELECT name, number, value FROM test WHERE number=?", database);

    auto values = statement.structValues<Output, Utils::SmallString, Utils::SmallString, long long>(3, queryValues);

    ASSERT_THAT(values, ElementsAre(Output{"poo", "40", 3},
                                    Output{"foo", "23.3", 2}));
}

TEST_F(SqliteStatement, GetStructOutputValuesAndContainerQueryTupleValues)
{
    using Tuple = std::tuple<Utils::SmallString, Utils::SmallString, int>;
    std::vector<Tuple> queryValues = {{"poo", "40", 3}, {"bar", "blah", 1}};
    ReadStatement statement("SELECT name, number, value FROM test WHERE name= ? AND number=? AND value=?", database);

    auto values = statement.structValues<Output, Utils::SmallString, Utils::SmallString, long long>(3, queryValues);

    ASSERT_THAT(values, ElementsAre(Output{"poo", "40", 3},
                                    Output{"bar", "blah", 1}));
}

void SqliteStatement::SetUp()
{
    database.setJournalMode(JournalMode::Memory);
    database.open(":memory:");
    database.execute("CREATE TABLE test(name TEXT UNIQUE, number NUMERIC, value NUMERIC)");
    database.execute("INSERT INTO  test VALUES ('bar', 'blah', 1)");
    database.execute("INSERT INTO  test VALUES ('foo', 23.3, 2)");
    database.execute("INSERT INTO  test VALUES ('poo', 40, 3)");
}

void SqliteStatement::TearDown()
{
    database.close();
}

}
