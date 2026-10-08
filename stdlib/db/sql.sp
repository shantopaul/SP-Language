module std.db.sql;

// ============================================================================
// Standard Library Fluent Query Builder (SP 2.5)
// ============================================================================

public class QueryBuilder {
    public string tableName;
    public string selectedCols;
    public string whereSqlite;
    public string wherePostgres;
    public int whereCount;
    public List<string> paramList;
    public string orderByCol;
    public string orderDir;
    public int limitCount;
    public int offsetCount;

    public QueryBuilder() {
        this.tableName = "";
        this.selectedCols = "*";
        this.whereSqlite = "";
        this.wherePostgres = "";
        this.whereCount = 0;
        this.paramList = new List<string>();
        this.orderByCol = "";
        this.orderDir = "ASC";
        this.limitCount = -1;
        this.offsetCount = -1;
    }

    public static fn from(string table) -> QueryBuilder {
        let qb = new QueryBuilder();
        qb.tableName = table;
        return qb;
    }

    public fn table(string name) -> QueryBuilder {
        this.tableName = name;
        return this;
    }

    public fn select(string cols = "*") -> QueryBuilder {
        this.selectedCols = cols;
        return this;
    }

    public fn where(string column, string op, string val) -> QueryBuilder {
        this.paramList.push(val);
        this.whereCount = this.whereCount + 1;
        let pIndex = this.whereCount;

        if (this.whereSqlite == "") {
            this.whereSqlite = " WHERE " + column + " " + op + " ?";
            this.wherePostgres = " WHERE " + column + " " + op + " $" + pIndex;
        } else {
            this.whereSqlite = this.whereSqlite + " AND " + column + " " + op + " ?";
            this.wherePostgres = this.wherePostgres + " AND " + column + " " + op + " $" + pIndex;
        }
        return this;
    }

    public fn whereEq(string column, string val) -> QueryBuilder {
        return this.where(column, "=", val);
    }

    public fn orderBy(string column, string dir = "ASC") -> QueryBuilder {
        this.orderByCol = column;
        this.orderDir = dir;
        return this;
    }

    public fn limit(int n) -> QueryBuilder {
        this.limitCount = n;
        return this;
    }

    public fn offset(int n) -> QueryBuilder {
        this.offsetCount = n;
        return this;
    }

    public fn toSelectSql(string dialect = "sqlite") -> string {
        let sql = "SELECT " + this.selectedCols + " FROM " + this.tableName;

        if (dialect == "postgres") {
            sql = sql + this.wherePostgres;
        } else {
            sql = sql + this.whereSqlite;
        }

        if (this.orderByCol != "") {
            sql = sql + " ORDER BY " + this.orderByCol + " " + this.orderDir;
        }

        if (this.limitCount >= 0) {
            sql = sql + " LIMIT " + this.limitCount;
        }

        if (this.offsetCount >= 0) {
            sql = sql + " OFFSET " + this.offsetCount;
        }

        sql = sql + ";";
        return sql;
    }

    public fn toDeleteSql(string dialect = "sqlite") -> string {
        let sql = "DELETE FROM " + this.tableName;

        if (dialect == "postgres") {
            sql = sql + this.wherePostgres;
        } else {
            sql = sql + this.whereSqlite;
        }

        sql = sql + ";";
        return sql;
    }

    public fn getParams() -> List<string> {
        return this.paramList;
    }
}
