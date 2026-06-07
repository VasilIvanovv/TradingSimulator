
+-------------------+
                     |    PriceCandle    |
                     +-------------------+
                               |
                               v
                     +-----------------------------------+
                     |            DataBroker             |
                     +-----------------------------------+
                       /                               \
        Holds a vector/                                 \ Holds a direct reference
      of web fetchers/                                   \ to the smart cache contract
                    v                                     v
          +------------------+                   +------------------+
          |  IDataProvider   |                   |    ILocalCache   | <--- The Storage Contract
          +------------------+                   +------------------+
            /              \                               |
           v                v                              v
    +--------------+  +------------+             +------------------+
    |TwelveDataProv|  | AlpacaProv |             |  CSVLocalCache  | <--- The CSV Writer/Reader

                     1. The Core Data Layout (Plain Old Data)
