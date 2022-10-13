-- postgresql/schemas/db-1/v003-user.sql

CREATE TYPE hello_schema.user AS (
	name TEXT,
	type hello_schema.user_type
);
