/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2016 The EVEmu Team
    For the latest information visit http://evemu.org
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:        Zhur
*/

#include "eve-server.h"

#include "market/MarketDB.h"

PyRep *MarketDB::GetStationAsks(uint32 stationID) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        "    typeID, MAX(price) AS price, volRemaining, stationID "
        " FROM srvMarket_orders "
        //" WHERE stationID=%u AND bid=%d"
        " WHERE stationID=%u"
        " GROUP BY typeID"
        " ,price, volRemaining, stationID",
            stationID/*, TransactionTypeSell*/))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return(DBResultToRowDict(res, "typeID"));
}

PyRep *MarketDB::GetSystemAsks(uint32 solarSystemID) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        "    typeID, MAX(price) AS price, volRemaining, stationID "
        " FROM srvMarket_orders "
        //" WHERE solarSystemID=%u AND bid=0"
        " WHERE solarSystemID=%u"
        " GROUP BY typeID"
        " ,price, volRemaining, stationID",
        solarSystemID))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return(DBResultToRowDict(res, "typeID"));
}

PyRep *MarketDB::GetRegionBest(uint32 regionID) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        "    typeID, MIN(price) AS price, volRemaining, stationID "
        " FROM srvMarket_orders "
        " WHERE regionID=%u AND bid=%d"
        " GROUP BY typeID"
        " ,price, volRemaining, stationID",
            regionID, TransactionTypeSell))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return(DBResultToRowDict(res, "typeID"));
}

PyRep *MarketDB::GetOrders( uint32 regionID, uint32 typeID )
{
    DBQueryResult res;

    PyList* tup = new PyList();

    /*DBColumnTypeMap colmap;
    colmap["volRemaining"] = DBTYPE_R8;
    colmap["price"] = DBTYPE_CY;
    colmap["issued"] = DBTYPE_FILETIME;

    colmap["orderID"] = DBTYPE_I4;
    colmap["volEntered"] = DBTYPE_I4;
    colmap["minVolume"] = DBTYPE_I4;
    colmap["stationID"] = DBTYPE_I4;
    colmap["regionID"] = DBTYPE_I4;
    colmap["solarSystemID"] = DBTYPE_I4;
    colmap["jumps"] = DBTYPE_I4;

    colmap["duration"] = DBTYPE_I2;
    colmap["typeID"] = DBTYPE_I2;
    colmap["range"] = DBTYPE_I2;

    colmap["bid"] = DBTYPE_BOOL;

    //ordering: (painstakingly determined from packets)
    DBColumnOrdering ordering;
    ordering.push_back("price");
    ordering.push_back("volRemaining");
    ordering.push_back("issued");
    ordering.push_back("orderID");
    ordering.push_back("volEntered");
    ordering.push_back("minVolume");
    ordering.push_back("stationID");
    ordering.push_back("regionID");
    ordering.push_back("solarSystemID");
    ordering.push_back("jumps");    //not working right...
    ordering.push_back("typeID");
    ordering.push_back("range");
    ordering.push_back("duration");
    ordering.push_back("bid");*/

    //query sell orders
    //TODO: consider the `jumps` field... is it actually used? might be a pain in the ass if we need to actually populate it based on each queryier's location
    if(!DBcore::RunQuery(res,
        "SELECT"
        "    price, volRemaining, typeID, `range`, orderID,"
        "   volEntered, minVolume, bid, issued as issueDate, duration,"
        "   stationID, regionID, solarSystemID, jumps"
        " FROM srvMarket_orders "
        " WHERE regionID=%u AND typeID=%u AND bid=%d", regionID, typeID, TransactionTypeSell))
    {
        codelog( MARKET__ERROR, "Error in query: %s", res.error.c_str() );

        PyDecRef( tup );
        return NULL;
    }
    SysLog::Debug("MarketDB::GetOrders", "Fetched %d sell orders for type %d", res.GetRowCount(), typeID);

    //this is wrong.
    tup->AddItem( DBResultToCRowset( res ) );

    //query buy orders
    if(!DBcore::RunQuery(res,
        "SELECT"
        "    price, volRemaining, typeID, `range`, orderID,"
        "   volEntered, minVolume, bid, issued as issueDate, duration,"
        "   stationID, regionID, solarSystemID, jumps"
        " FROM srvMarket_orders "
        " WHERE regionID=%u AND typeID=%u AND bid=%d", regionID, typeID, TransactionTypeBuy))
    {
        codelog( MARKET__ERROR, "Error in query: %s", res.error.c_str() );

        PyDecRef( tup );
        return NULL;
    }
    SysLog::Debug("MarketDB::GetOrders", "Fetched %d buy orders for type %d", res.GetRowCount(), typeID);

    //this is wrong.
    tup->AddItem( DBResultToCRowset( res ) );

    return tup;
}

PyRep *MarketDB::GetCharOrders(uint32 characterID) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        "   orderID, typeID, charID, regionID, stationID,"
        "   `range`, bid, price, volEntered, volRemaining,"
        "   issued as issueDate, orderState, minVolume, contraband,"
        "   accountID, duration, isCorp, solarSystemID,"
        "   escrow"
        " FROM srvMarket_orders "
        " WHERE charID=%u", characterID))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return DBResultToRowset(res);
}

PyRep *MarketDB::GetOrderRow(uint32 orderID) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        "    price, volRemaining, typeID, `range`, orderID,"
        "   volEntered, minVolume, bid, issued as issueDate, duration,"
        "   stationID, regionID, solarSystemID, jumps"
        " FROM srvMarket_orders"
        " WHERE orderID=%u", orderID))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    DBResultRow row;
    if(!res.GetRow(row)) {
        codelog(MARKET__ERROR, "Order %u not found.", orderID);
        return NULL;
    }

    return(DBRowToPackedRow(row));
}

PyRep *MarketDB::GetOldPriceHistory(uint32 regionID, uint32 typeID) {
    DBQueryResult res;

    /*DBColumnTypeMap colmap;
    colmap["historyDate"] = DBTYPE_FILETIME;
    colmap["lowPrice"] = DBTYPE_CY;
    colmap["highPrice"] = DBTYPE_CY;
    colmap["avgPrice"] = DBTYPE_CY;
    colmap["volume"] = DBTYPE_I8;
    colmap["orders"] = DBTYPE_I4;

    //ordering: (painstakingly determined from packets)
    DBColumnOrdering ordering;
    ordering.push_back("historyDate");
    ordering.push_back("lowPrice");
    ordering.push_back("highPrice");
    ordering.push_back("avgPrice");
    ordering.push_back("volume");
    ordering.push_back("orders");*/

    if(!DBcore::RunQuery(res,
        "SELECT"
        "    historyDate, lowPrice, highPrice, avgPrice,"
        "    volume, orders "
        " FROM srvMarket_history_old "
        " WHERE regionID=%u AND typeID=%u", regionID, typeID))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return(DBResultToCRowset(res));
}

PyRep *MarketDB::GetNewPriceHistory(uint32 regionID, uint32 typeID) {
    DBQueryResult res;

    /*DBColumnTypeMap colmap;
    colmap["historyDate"] = DBTYPE_FILETIME;
    colmap["lowPrice"] = DBTYPE_CY;
    colmap["highPrice"] = DBTYPE_CY;
    colmap["avgPrice"] = DBTYPE_CY;
    colmap["volume"] = DBTYPE_I8;
    colmap["orders"] = DBTYPE_I4;

    //ordering: (painstakingly determined from packets)
    DBColumnOrdering ordering;
    ordering.push_back("historyDate");
    ordering.push_back("lowPrice");
    ordering.push_back("highPrice");
    ordering.push_back("avgPrice");
    ordering.push_back("volume");
    ordering.push_back("orders");*/

    //build the history record from the recent market transactions.
    //NOTE: it may be a good idea to cache the historyDate column in each
    //record when they are inserted instead of re-calculating it each query.
    // this would also allow us to put together an index as well...
    if(!DBcore::RunQuery(res,
        "SELECT"
        "    transactionDateTime - ( transactionDateTime %% %" PRId64 " ) AS historyDate,"
        "    MIN(price) AS lowPrice,"
        "    MAX(price) AS highPrice,"
        "    AVG(price) AS avgPrice,"
        "    CAST(SUM(quantity) AS SIGNED INTEGER) AS volume,"
        "    CAST(COUNT(transactionID) AS SIGNED INTEGER) AS orders"
        " FROM srvMarket_transactions "
        " WHERE regionID=%u AND typeID=%u"
        "    AND transactionType=%d "    //both buy and sell transactions get recorded, only compound one set of data... choice was arbitrary.
        " GROUP BY historyDate"
        " ,lowPrice, highPrice, avgPrice, volume, orders",
        Win32Time_Day, regionID, typeID, TransactionTypeBuy))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return(DBResultToCRowset(res));
}

bool MarketDB::BuildOldPriceHistory() {
    DBerror err;

    uint64 cutoff_time = Win32TimeNow();
    cutoff_time -= cutoff_time % Win32Time_Day;    //round down to an even day boundary.
    cutoff_time -= HISTORY_AGGREGATION_DAYS * Win32Time_Day;

    //build the history record from the recent market transactions.
    if(!DBcore::RunQuery(err,
        "INSERT INTO"
        "    srvMarket_history_old"
        "     (regionID, typeID, historyDate, lowPrice, highPrice, avgPrice, volume, orders)"
        " SELECT"
        "    regionID,"
        "    typeID,"
        "    transactionDateTime - ( transactionDateTime %% %" PRId64 " ) AS historyDate,"
        "    MIN(price) AS lowPrice,"
        "    MAX(price) AS highPrice,"
        "    AVG(price) AS avgPrice,"
        "    SUM(quantity) AS volume,"
        "    COUNT(transactionID) AS orders"
        " FROM srvMarket_transactions "
        " WHERE"
        "    transactionType=%d AND "    //both buy and sell transactions get recorded, only compound one set of data... choice was arbitrary.
        "    ( transactionDateTime - ( transactionDateTime %% %" PRId64 " ) ) < %" PRId64
        " GROUP BY regionID, typeID, historyDate"
        " ,lowPrice, highPrice, avgPrice, volume, orders",
            Win32Time_Day,
            TransactionTypeBuy,
            Win32Time_Day,
            cutoff_time
            ))
    {
        codelog(MARKET__ERROR, "Error in query: %s", err.c_str());
        return false;
    }

    //now remove the transactions which have been aged out?
    if(!DBcore::RunQuery(err,
        "DELETE FROM"
        "    srvMarket_transactions"
        " WHERE"
        "    transactionDateTime < %" PRId64,
        cutoff_time))

    {
        codelog(MARKET__ERROR, "Error in query: %s", err.c_str());
        return false;
    }

    return true;
}
PyObject *MarketDB::GetCorporationBills(uint32 corpID, bool payable)
{
    DBQueryResult res;
    bool success = false;

    if ( payable == true )
    {
        success = DBcore::RunQuery(res, "SELECT billID, billTypeID, debtorID, creditorID, amount, dueDateTime, interest,"
            "externalID, paid externalID2 FROM srvBillsPayable WHERE debtorID = %u", corpID);
    }
    else
    {
        success = DBcore::RunQuery(res, "SELECT billID, billTypeID, debtorID, creditorID, amount, dueDateTime, interest,"
            "externalID, paid externalID2 FROM srvBillsReceivable WHERE creditorID = %u", corpID);
    }

    if ( success == false )
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return DBResultToRowset(res);
}

PyObject *MarketDB::GetRefTypes() {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        " billTypeID,"
        " billTypeName,"
        " description"
        " FROM blkBillTypes"
    )) {
        _log(DATABASE__ERROR, "Failed to query bill types: %s.", res.error.c_str());
        return NULL;
    }

    return DBResultToRowset(res);
}

//this is a crap load of work... there HAS to be a better way to do this..
PyRep *MarketDB::GetMarketGroups() {
    
    DBQueryResult res;
    DBResultRow row;

    if (!DBcore::RunQuery(res,
                          "SELECT parentGroupID, marketGroupID, marketGroupName, "
                          "IFNULL(description, ''), graphicID, hType AS hasTypes, iconID, dataID, "
                          "marketGroupNameID, descriptionID "
                          " FROM invMarketGroups LEFT JOIN extInvMarketGroups USING(marketGroupID)"))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    DBRowDescriptor *header = new DBRowDescriptor(res);
    
    CFilterRowSet *filterRowset = new CFilterRowSet(&header);
    
    PyDict *keywords = filterRowset->GetKeywords();
	keywords->SetItemString("giveMeSets", new PyBool(false)); //+
	keywords->SetItemString("allowDuplicateCompoundKeys", new PyBool(false)); //+
	keywords->SetItemString("indexName", new PyNone); //+
	keywords->SetItemString("columnName", new PyString("parentGroupID")); //+
    std::map< int, PyRep* > tt;

    while( res.GetRow(row) )
    {
        int parentGroupID = ( row.IsNull( 0 ) ? -1 : row.GetUInt( 0 ) );
        PyRep *pid;
        CRowSet *rowset;
        if(tt.count(parentGroupID) == 0) {
            pid = parentGroupID!=-1 ? (PyRep*)new PyInt(parentGroupID) : (PyRep*)new PyNone();
            tt.insert( std::pair<int, PyRep*>(parentGroupID, pid) );
            rowset = filterRowset->NewRowset(pid);
        } else {
            pid = tt[parentGroupID];
            rowset = filterRowset->GetRowset(pid);
        }
        
        PyPackedRow* pyrow = rowset->NewRow();

        pyrow->SetField((uint32)0, pid); //prentGroupID
        pyrow->SetField(1, new PyInt(row.GetUInt( 1 ) ) ); //marketGroupID
        pyrow->SetField(2, new PyString(row.GetText( 2 ) ) ); //marketGroupName
        pyrow->SetField(3, new PyString(row.GetText( 3 ) ) ); //description
        pyrow->SetField(4, row.IsNull( 4 ) ? 
            (PyRep*)(new PyNone()) : new PyInt(row.GetUInt( 4 ))  ); //graphicID
        pyrow->SetField(5, new PyBool((row.GetText(5))[0] != 0)); //hasTypes
        pyrow->SetField(6, row.IsNull( 6 ) ? 
            (PyRep*)(new PyNone()) : new PyInt(row.GetUInt( 6 ))  ); // iconID 
        pyrow->SetField(7, new PyInt( row.GetUInt(7) )  ); //dataID
        pyrow->SetField(8, new PyInt( row.GetUInt(8) )  ); //marketGroupNameID
        pyrow->SetField(9, new PyInt( row.GetUInt(9) )  ); //descriptionID
    }

    return filterRowset;
}

uint32 MarketDB::StoreBuyOrder(
    uint32 clientID,
    uint32 accountID,
    uint32 stationID,
    uint32 typeID,
    double price,
    uint32 quantity,
    uint8 orderRange,
    uint32 minVolume,
    uint8 duration,
    bool isCorp
) {
    return(_StoreOrder(clientID, accountID, stationID, typeID, price, quantity, orderRange, minVolume, duration, isCorp, true));
}

uint32 MarketDB::StoreSellOrder(
    uint32 clientID,
    uint32 accountID,
    uint32 stationID,
    uint32 typeID,
    double price,
    uint32 quantity,
    uint8 orderRange,
    uint32 minVolume,
    uint8 duration,
    bool isCorp
) {
    return(_StoreOrder(clientID, accountID, stationID, typeID, price, quantity, orderRange, minVolume, duration, isCorp, false));
}

//NOTE: needs a lot of work to implement orderRange
uint32 MarketDB::FindBuyOrder(
    uint32 stationID,
    uint32 typeID,
    double price,
    uint32 quantity,
    uint32 orderRange
) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT orderID"
        "    FROM srvMarket_orders"
        "    WHERE bid=1"
        "        AND typeID=%u"
        "        AND stationID=%u"
        "        AND volRemaining >= %u"
        "        AND price <= %f"
        "    ORDER BY price DESC"
        "    LIMIT 1",    //right now, we just care about the first order which can satisfy our needs.
        typeID,
        stationID,
        quantity,
        price))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if(!res.GetRow(row))
        return(0);    //no order found.

    return(row.GetUInt(0));
}

uint32 MarketDB::FindSellOrder(
    uint32 stationID,
    uint32 typeID,
    double price,
    uint32 quantity,
    uint32 orderRange
) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT orderID"
        "    FROM srvMarket_orders"
        "    WHERE bid=0"
        "        AND typeID=%u"
        "        AND stationID=%u"
        "        AND volRemaining >= %u"
        "        AND price <= %f"
        "    ORDER BY price ASC"
        "    LIMIT 1",    //right now, we just care about the first order which can satisfy our needs.
        typeID,
        stationID,
        quantity,
        price))
    {
        codelog(MARKET__ERROR, "Error in query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if(!res.GetRow(row))
        return(0);    //no order found.

    return(row.GetUInt(0));
}

bool MarketDB::GetOrderInfo(uint32 orderID, uint32 *orderOwnerID, uint32 *typeID, uint32 *stationID, uint32 *quantity, double *price, bool *isBuy, bool *isCorp) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        " volRemaining,"
        " price,"
        " typeID,"
        " stationID,"
        " charID,"
        " bid,"
        " isCorp"
        " FROM srvMarket_orders"
        " WHERE orderID=%u",
        orderID))
    {
        _log(MARKET__ERROR, "Error in query: %s.", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if(!res.GetRow(row)) {
        _log(MARKET__ERROR, "Order %u not found.", orderID);
        return false;
    }

    if(quantity != NULL)
        *quantity = row.GetUInt(0);
    if(price != NULL)
        *price = row.GetDouble(1);
    if(typeID != NULL)
        *typeID = row.GetUInt(2);
    if(stationID != NULL)
        *stationID = row.GetUInt(3);
    if(orderOwnerID != NULL)
        *orderOwnerID = row.GetUInt(4);
    if(isBuy != NULL)
        *isBuy = row.GetInt(5) ? true : false;
    if(isCorp != NULL)
        *isCorp = row.GetInt(6) ? true : false;

    return true;
}

//NOTE: this logic needs some work if there are multiple concurrent market services running at once.
bool MarketDB::AlterOrderQuantity(uint32 orderID, uint32 new_qty) {
    DBerror err;

    if(!DBcore::RunQuery(err,
        "UPDATE"
        " srvMarket_orders"
        " SET volRemaining = %u"
        " WHERE orderID = %u",
        new_qty, orderID))
    {
        _log(MARKET__ERROR, "Error in query: %s.", err.c_str());
        return false;
    }

    return true;
}

bool MarketDB::AlterOrderPrice(uint32 orderID, double new_price) {
    DBerror err;

    if(!DBcore::RunQuery(err,
        "UPDATE"
        " srvMarket_orders"
        " SET price = %f"
        " WHERE orderID = %u",
        new_price, orderID))
    {
        _log(MARKET__ERROR, "Error in query: %s.", err.c_str());
        return false;
    }

    return true;
}

bool MarketDB::DeleteOrder(uint32 orderID) {
    DBerror err;

    if(!DBcore::RunQuery(err,
        "DELETE"
        " FROM srvMarket_orders"
        " WHERE orderID = %u",
        orderID))
    {
        _log(MARKET__ERROR, "Error in query: %s.", err.c_str());
        return false;
    }

    return true;
}

bool MarketDB::AddCharacterBalance(uint32 char_id, double delta)
{
    DBerror err;

    if(!DBcore::RunQuery(err,
        "UPDATE srvCharacter SET balance=balance+%.2f WHERE characterID=%u",delta,char_id))
    {
        _log(SERVICE__ERROR, "Error in query : %s", err.c_str());
        return false;
    }

    return (true);
}

bool MarketDB::RecordTransaction(
    uint32 typeID,
    uint32 quantity,
    double price,
    MktTransType transactionType,
    uint32 charID,
    uint32 regionID,
    uint32 stationID
) {
    DBerror err;

    if(!DBcore::RunQuery(err,
        "INSERT INTO"
        " srvMarket_transactions ("
        "    transactionID, transactionDateTime, typeID, quantity,"
        "    price, transactionType, clientID, regionID, stationID,"
        "    corpTransaction"
        " ) VALUES ("
        "    NULL, %" PRIu64 ", %u, %u,"
        "    %f, %d, %u, %u, %u, 0"
        " )",
            Win32TimeNow(), typeID, quantity,
            price, transactionType, charID, regionID, stationID
            ))
    {
        codelog(MARKET__ERROR, "Error in query: %s", err.c_str());
        return false;
    }
    return true;
}

uint32 MarketDB::_StoreOrder(
    uint32 clientID,
    uint32 accountID,
    uint32 stationID,
    uint32 typeID,
    double price,
    uint32 quantity,
    uint8 orderRange,
    uint32 minVolume,
    uint8 duration,
    bool isCorp,
    bool isBuy
) {
    DBerror err;

    uint32 solarSystemID;
    uint32 regionID;
    if(!GetStationInfo(stationID, &solarSystemID, NULL, &regionID, NULL, NULL, NULL)) {
        codelog(MARKET__ERROR, "Char %u: Failed to find parents for station %u", clientID, stationID);
        return(0);
    }

    //TODO: figure out what the orderState field means...
    //TODO: implement the contraband flag properly.
    //TODO: implement the isCorp flag properly.
    uint32 orderID;
    if(!DBcore::RunQueryLID(err, orderID,
        "INSERT INTO srvMarket_orders ("
        "    typeID, charID, regionID, stationID,"
        "    `range`, bid, price, volEntered, volRemaining, issued,"
        "    orderState, minVolume, contraband, accountID, duration,"
        "    isCorp, solarSystemID, escrow, jumps "
        " ) VALUES ("
        "    %u, %u, %u, %u, "
        "    %u, %u, %f, %u, %u, %" PRIu64 ", "
        "    1, %u, 0, %u, %u, "
        "    %u, %u, 0, 1"
        " )",
            typeID, clientID, regionID, stationID,
            orderRange, isBuy?1:0, price, quantity, quantity, Win32TimeNow(),
            minVolume, accountID, duration,
            isCorp?1:0, solarSystemID
        ))

    {
        codelog(MARKET__ERROR, "Error in query: %s", err.c_str());
        return(0);
    }

    return(orderID);
}

PyRep *MarketDB::GetTransactions(uint32 characterID, uint32 typeID, uint32 quantity, double minPrice, double maxPrice, uint64 fromDate, int buySell)
{
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        " transactionID,transactionDateTime,typeID,quantity,price,transactionType,"
        " 0 AS corpTransaction,clientID,stationID"
        " FROM srvMarket_transactions "
        " WHERE clientID=%u AND (typeID=%u OR 0=%u) AND"
        " quantity>=%u AND price>=%f AND (price<=%f OR 0=%f) AND"
        " transactionDateTime>=%" PRIu64 " AND (transactionType=%d OR -1=%d)",
        characterID, typeID, typeID, quantity, minPrice, maxPrice, maxPrice, fromDate, buySell, buySell))
    {
        codelog( MARKET__ERROR, "Error in query: %s", res.error.c_str() );

        return NULL;
    }

    return DBResultToRowset(res);
}
