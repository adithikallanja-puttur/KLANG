@echo off
echo ====================================================
echo Starting Portable MySQL/MariaDB Server on port 3306
echo ====================================================
start "" "C:\Users\hp\mysql_server\mariadb-11.4.3-winx64\bin\mysqld.exe" --datadir="C:\Users\hp\mysql_server\data" --port=3306
timeout /t 2 /nobreak >nul
echo MySQL is running on 127.0.0.1:3306 (user: root, no password).
echo You can run 'klang.exe mysql_test.klang' now!
