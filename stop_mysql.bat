@echo off
echo ====================================================
echo Stopping MySQL/MariaDB Server...
echo ====================================================
"C:\Users\hp\mysql_server\mariadb-11.4.3-winx64\bin\mariadb-admin.exe" -u root shutdown
echo MySQL server has stopped.
