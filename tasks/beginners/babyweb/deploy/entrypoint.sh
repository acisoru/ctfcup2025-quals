#!/bin/ash

MYSQL_USER=ctfmarket_user
MYSQL_PASSWORD=ctfmarket_pass
MYSQL_HOST=mariadb
MYSQL_DATABASE=ctfmarket_db

while ! mysqladmin ping -u${MYSQL_USER} -p${MYSQL_PASSWORD} -h${MYSQL_HOST} --silent; do echo "mysqld is not yet alive" && sleep .20; done

mysql -u${MYSQL_USER} -p${MYSQL_PASSWORD} -h${MYSQL_HOST} << EOF
CREATE TABLE IF NOT EXISTS ${MYSQL_DATABASE}.accounts (
  id INT UNSIGNED AUTO_INCREMENT NOT NULL,
  email varchar(255) NOT NULL,
  password_hash varchar(255) NOT NULL,
  coins DECIMAL(10,5) UNSIGNED NOT NULL,
  purchased BOOLEAN NOT NULL DEFAULT 0,
  PRIMARY KEY (id)
) AUTO_INCREMENT=500001337;
CREATE TABLE IF NOT EXISTS ${MYSQL_DATABASE}.shopping_basket (
  id INT UNSIGNED AUTO_INCREMENT NOT NULL,
  account_id INT NOT NULL,
  products TEXT NOT NULL,
  total_price FLOAT UNSIGNED NOT NULL,
  currency_code varchar(10) NOT NULL,
  payment_status BOOLEAN NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  UNIQUE (account_id)
);
EOF

echo `date`': supervisord will be launched right now'

/usr/bin/supervisord -c /etc/supervisord.conf
