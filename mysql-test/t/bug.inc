eval CREATE TABLE t1 ( pk VARCHAR(16) NOT NULL PRIMARY KEY ) CHARSET=$charset COLLATE=$collation;

INSERT INTO t1 VALUES ('abc'), ('def'), ('ghi'), ('jkl'), ('mno');
ANALYZE TABLE t1;

EXPLAIN
SELECT * FROM t1 WHERE pk IN ('abc', 'this_value_is_>16', 'def');
SELECT * FROM t1 WHERE pk IN ('abc', 'this_value_is_>16', 'def');

EXPLAIN
SELECT * FROM t1 WHERE pk IN ('abc', 'this_value_is_>16', 'def');
SELECT * FROM t1 WHERE pk IN ('abc', 'this_value_is_>16', 'def');

EXPLAIN
SELECT * FROM t1 WHERE pk IN ('this_value_is_>16');
SELECT * FROM t1 WHERE pk IN ('this_value_is_>16');

EXPLAIN
SELECT * FROM t1 WHERE pk IN ('this_value_is_>16', 'and_so_is_this_one');
SELECT * FROM t1 WHERE pk IN ('this_value_is_>16', 'and_so_is_this_one');


eval CREATE TABLE t2 ( a VARCHAR(1), KEY (a) ) CHARSET=$charset COLLATE=$collation;

INSERT INTO t2 VALUES (NULL), ('a'), ('b'), ('æ'), ('ß'), ('s');
INSERT INTO t2 SELECT 'x' FROM t2;
INSERT INTO t2 SELECT 'x' FROM t2;

ANALYZE TABLE t2;

EXPLAIN
SELECT * FROM t2 WHERE a IN ('ae', 'ss', CONCAT(_utf16 x'00AD', 'b'));
SELECT * FROM t2 WHERE a IN ('ae', 'ss', CONCAT(_utf16 x'00AD', 'b'));

EXPLAIN
SELECT * FROM t2 WHERE a IN ('ss', CONCAT(_utf16 x'00AD', 'ae'));
SELECT * FROM t2 WHERE a IN ('ss', CONCAT(_utf16 x'00AD', 'ae'));

EXPLAIN
SELECT * FROM t2 WHERE a IN ('ss', CONCAT(_utf16 x'00AD', 'ae', _utf16 x'00AD'));
SELECT * FROM t2 WHERE a IN ('ss', CONCAT(_utf16 x'00AD', 'ae', _utf16 x'00AD'));

EXPLAIN
SELECT * FROM t2 WHERE a = 'ae' OR a = 'ss' OR a = CONCAT(_utf16 x'00AD', 'b');
SELECT * FROM t2 WHERE a = 'ae' OR a = 'ss' OR a = CONCAT(_utf16 x'00AD', 'b');


eval CREATE TABLE t3 ( a VARCHAR(768), KEY(a)) CHARSET=$charset COLLATE=$collation;

INSERT INTO t3 VALUES( REPEAT('æ', 767) ), ( REPEAT('æ', 768) );

ANALYZE TABLE t3;

EXPLAIN
SELECT 1 FROM t3 WHERE a IN ( 'a', REPEAT('æ', 767) );
SELECT 1 FROM t3 WHERE a IN ( 'a', REPEAT('æ', 768) );

EXPLAIN
SELECT 1 FROM t3 WHERE a IN ( 'a', REPEAT('ae', 767) );
SELECT 1 FROM t3 WHERE a IN ( 'a', REPEAT('ae', 768) );


eval CREATE TABLE t4 ( a VARCHAR(100), KEY(a(50))) CHARSET=$charset COLLATE=$collation;

INSERT INTO t4 VALUES( REPEAT('æ', 98) ), ( REPEAT('æ', 99) ), ( REPEAT('æ', 100) );

ANALYZE TABLE t4;

EXPLAIN
SELECT 1 FROM t4 WHERE a IN ( 'a', REPEAT('æ', 98) );
SELECT 1 FROM t4 WHERE a IN ( 'a', REPEAT('æ', 98) );


eval CREATE TABLE t5 ( a VARCHAR(16383), KEY(a(768))) CHARSET=$charset COLLATE=$collation;

INSERT INTO t5 VALUES( REPEAT('æ', 16383) ), ( REPEAT('æ', 16382) ), ( REPEAT('æ', 16381) );

ANALYZE TABLE t5;

EXPLAIN
SELECT 1 FROM t5 WHERE a IN ( 'a', REPEAT('æ', 16381) );
SELECT 1 FROM t5 WHERE a IN ( 'a', REPEAT('æ', 16381) );

EXPLAIN
SELECT 1 FROM t5 WHERE a IN ( 'a', REPEAT('æ', 16382) );
SELECT 1 FROM t5 WHERE a IN ( 'a', REPEAT('æ', 16382) );

EXPLAIN
SELECT 1 FROM t5 WHERE a IN ( 'a', REPEAT('æ', 16383) );
SELECT 1 FROM t5 WHERE a IN ( 'a', REPEAT('æ', 16383) );


DROP TABLE t1, t2, t3, t4, t5;
