import subprocess

result = subprocess.Popen(["php /var/www/html/SQL_toESP-A.php"], shell=True, stdout=subprocess.PIPE)

script_response = result.stdout.read()
print(script_response)