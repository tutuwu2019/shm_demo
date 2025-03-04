-- create_db.sql
CREATE TABLE IF NOT EXISTS config_data (
    id INTEGER PRIMARY KEY,
    key TEXT NOT NULL,
    value TEXT NOT NULL
);

INSERT INTO config_data (key, value) VALUES
('timeout', '30'),
('max_connections', '1000'),
('retry_count', '5');
