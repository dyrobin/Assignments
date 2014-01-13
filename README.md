T-110.5150 Application and Services in Internet
===========================
Materials for Assignments and Course Staff

URL to course Git group:
https://git.niksula.hut.fi/groups/t-110-5150

CONTENTS
===========================

  p2pn/ : Sample implementation of Peer-to-Peer assignment.
          Including stripped headers that are supplied to students as
          reference.
  *.tex : Assignment instructions.
  group.csv : Group sign-up book.
  keys.csv : Amazon EC2 credentials.


  TeX -> PDF

  To typeset the TeX files, clone the 'opastus' repository on any
  Aalto-IT Linux workstations (in Maarintalo, for example) and type:

   $ pdflatex <name>.tex

  and you get the PDF file.



AT THE END OF THE SEMESTER
===========================

 1. Clone, pack, and send the 'opastus' repository to the teacher,
    so that it can be passed on to future course staff.

FOR FUTURE COURSE STAFF
===========================

 1. Please contact Niksula admin to transfer the ownership of course
    Git group to you.
    If the previous course staff is still reachable you may also ask
    them to do the transfer.
 2. Review and update materials in 'opastus' repository.
    Re-create the reference header p2pn/p2p.h if necessary.
    Check TODOs in individual documents.
 3. Update groups.csv according to the sign up list of this year.
 4. Create repositories for this year's students.
 5. EC2: Create accounts for students and update keys.csv .

