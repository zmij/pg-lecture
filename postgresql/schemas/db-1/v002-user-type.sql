-- postgresql/db-1/v002-user-type.sql

CREATE TYPE hello_schema.user_type AS ENUM (
	'first_time',
	'known'
);