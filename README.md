T-110.5150 Application and Services in Internet
===========================
Materials for Assignments and Course Staff

Repositories for student assugnments and course materials are hosted using Niksula Git (http://www.niksula.hut.fi/git).

- URL to course Git group:
 https://git.niksula.hut.fi/groups/t-110-5150

- URL to this repository:
 https://git.niksula.hut.fi/t-110-5150/opastus

- Important documentations:
 http://rtfm.cs.hut.fi/
 http://www.niksula.hut.fi/


FOR COURSE STAFF
===========================

  - Please contact Niksula Guru (http://www.niksula.hut.fi/staff) to transfer the ownership of course Git group (the group, not individual repositories) to you.
    If the previous course staff is still reachable you may also ask them to do the transfer.
  - Assume Master access of `opastus` repository (this repository).
  - Review and update materials in this repository.
    Re-create the reference header `p2pn/p2p.h` if necessary.
    Check TODOs in individual documents.
  - Update `groups.csv` according to the sign up list of this year.
  - Create repositories for this year's students.
  - P2P: Apply for virtual machines for bootstrap servers from Niksula.
    See http://rtfm.cs.hut.fi/virtual-machines (Intranet web site).
    Note that there are two IT systems in the CSE department, **cs.hut** for *work* and **Niksula** for students.
    Once you signed the work contract for course assistant, you became an employee of the department.
    Check if you have an account for *work*: http://rtfm.cs.hut.fi/unix-guide .
    Plan the firewall access requirements carefully.
    P2P service ports need to be globally accessible.
  - EC2: Request Amazon management credentials from the teacher.
    Create accounts for students and update `keys.csv`.
  - Verify assignment instructions and sample implementations.
  - If you took the course before, verify also your previous experiences and solutions.


CONTENTS
===========================

  - `p2pn/` : Sample implementation of Peer-to-Peer assignment.
          Including stripped headers that are supplied to students as
          reference.
  - `*.tex` : Assignment instructions and demo sheet.
  - `group.csv` : Group sign-up book.
  - `keys.csv` : Amazon EC2 credentials.


  - TeX -> PDF

  To typeset the TeX files, clone this repository on any
  Aalto-IT Linux workstations (in Maarintalo, for example) and type:

   `pdflatex <name>.tex`

  and you get the PDF file.


AT THE END OF THE SEMESTER
===========================

 - Clean up and revise this repository.

