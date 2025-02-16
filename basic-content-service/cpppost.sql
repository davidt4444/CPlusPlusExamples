CREATE TABLE CPPPost (
    id INT AUTO_INCREMENT PRIMARY KEY,
    uniqueId CHAR(36) NOT NULL DEFAULT (UUID()),
    title VARCHAR(200) NOT NULL,
    content TEXT NOT NULL,
    createdAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    author VARCHAR(200),
    category VARCHAR(100),
    updatedAt DATETIME,
    likesCount INT NOT NULL DEFAULT 0,
    authorId INT,
    isPublished BOOLEAN NOT NULL DEFAULT TRUE,
    views INT NOT NULL DEFAULT 0,
    CHECK (CHAR_LENGTH(title) >= 5 AND CHAR_LENGTH(title) <= 200),
    CHECK (CHAR_LENGTH(content) <= 10000)  -- Assuming text length can be constrained this way, which might not be supported by all SQL dialects
);

-- Adding a foreign key constraint if 'authorId' is meant to reference another table 'User'.
-- Note: You'll need to adjust this if the actual table name or column differs.
ALTER TABLE CPPPost
ADD CONSTRAINT fk_author
FOREIGN KEY (authorId) REFERENCES User(id);

INSERT INTO CPPPost(
    title, 
    content, 
    author, 
    category, 
    createdAt
    ) VALUES (
        "A test article", 
        "Some content", 
        "Me", 
        "test_cat", 
        NOW()
        )
   ; 
INSERT INTO CPPPost(title, content, author, category, createdAt) VALUES (?, ?, ?, ?, NOW())

UPDATE CPPPost
SET title='A test article', 
    content='Some content', 
    author='Me', 
    category='test_cat', 
    updatedAt=NOW()
WHERE id=1
;
UPDATE CPPPost SET title=?, content=?, author=?, category=?, updatedAt=NOW() WHERE id=?

delete from CPPPost where id =1;
delete from CPPPost where id =?

