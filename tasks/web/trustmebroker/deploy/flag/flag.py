import requests
import os
import time
flag = os.getenv('FLAG')#"ctfcup{61d402f23f8f16637cff1c08f303bebc9a9eaf19abf056f988fcab69ef43}"
host = os.getenv('WEB_SERVICE_URL')

print(flag)

def split_string(input_string, chunk_size=17):
    return [input_string[i:i+chunk_size] for i in range(0, len(input_string), chunk_size)]

splited = split_string(flag)

headers = {
    'accept-language': 'ru,en-US;q=0.9,en;q=0.8,zh-TW;q=0.7,zh;q=0.6',
    'content-type': 'application/json'
}
json_data = {
    'lastname': 'Doe',
    'firstname': 'John',
    'middlename': '-',
    'birthdate': '1994-05-05',
    'email': 'john@doe.com',
    'passport_series': splited[0],
    'passport_number': splited[1],
    'passport_issued_by': splited[2],
    'passport_issued_date': splited[3],
    'address': '-',
    'agree': '',
}

while True:
    try:
        print("ping")
        resp = requests.get(host, timeout=5)
        if resp.status_code == 200:
            break
    except:
        pass
    time.sleep(2)
    
response = requests.post(host + '/register/step2', headers=headers, json=json_data)

ans = response.json()
if ans["ok"] == True:
    print('success', ans["agreement_id"])
else:
    print('error', response.text)

try:
    cookies = {"agreement_id":ans["agreement_id"]}
    response = requests.get(host + '/agreement', cookies=cookies)
    if response.status_code == 200:
        print('FLAG IS UP!')
    else:
        print('Something wrong, flag pdf response with ', response.status_code)
        print(response.text)
except Exception as e:
    print(e)
