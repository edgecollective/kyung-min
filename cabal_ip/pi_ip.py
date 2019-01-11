import subprocess
from random import randint
from subprocess import check_output


#key='ed7592aa67cb80c60b9f7a256250350a75e123104940555f8f678cf270b4818e'
key='aff26da71f1d8113f947bb4e2f0b4b32f9f462c41f8b6f364eeabdba45a4123f'
channel = 'pi_ip'
timeout = 10000

out=str(check_output(["hostname","-I"]))
ips=out.strip().split(' ')
#print(ips[0])

pi_ip=ips[0]

output=check_output(['cabal', '--message',str(pi_ip), '--key', str(key), '--channel', str(channel), '--timeout', str(timeout)])
