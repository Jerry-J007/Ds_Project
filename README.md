BlockChain fields:

roll.no
vote
timestamp
hash
reciept ( needed to check their votes in later on point)

The head node in LL is the first vote. Tail is where new nodes are added.



ballotI.db

line 1:   title|adminRoll|cand1,cand2,cand3
line 2:   roll,roll,roll
line 3+:  blockID|roll|vote|timestamp|prevHash|currHash


in case admin logins -> in web page itself redirect him to admin page.