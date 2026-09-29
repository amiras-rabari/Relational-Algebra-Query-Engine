# DESIGN_LOG

## 19/20 -Sept -2026

I first started by understanding what the project is actually asking me to build, and familiarized myself with some unfamiliar terminology using lecture notes and AI

## 20 -Sept 2026

I watched all the lectures up until this date, which in fact strengthened my SQL skills as I used SQL in one of my projects. 

After that, I again went over the assignment expectations and strict rules. AI was used to kind of answer what each step expects, the type of tools, and what they are. 

Finally, I decided to use C++ as my language for the project, as it gives me low-level control plus OOP design, and I wanted to learn a new language since C was way too much complication and Java was already familiar. I used C++ to push my boundaries and will be using CLion IDE to fasten the development process 

By the end of the day—meaning 2 AM the next morning 🤣 (I work late, started my lectures at 2 PM)—I was working on the EBNF structure and was still figuring out the exact structure

## 21 -Sept - 2026

Started reading Dragon book as i do love low level architecture decided to read chapter 1 and 2 for now and start building my grammar and  code . 

After a lot of consideration and looking at the structure of EBNF, I decided to co-create my grammar with the help of AI and using my knowledge acquired by the dragon book. The AI acted simply as a teacher, guiding me through the process. For instance, it would say, "First, I want you to define precedence and associativity." Then, I would provide my complete input in a single prompt, covering almost every edge case. Most of the time, my explanations covered about 90% of the edge cases. For the remaining gaps, the AI would explain why those specific cases were important. Once I understood, I moved on to the next step. This iterative approach gave me a clear roadmap of what I was doing at each stage. I was able to define my entire grammar after about 4 to 5 hours of trial and error, as well as reading extra documentation for languages like C, Java, and SQL to see what kind of precedence rules and conventions they follow. While I wouldn't say the final grammar is 100% my own isolated thought, I can confidently say that 90% of the implementation was my brainchild. The AI was simply there to refine my knowledge and bridge the gap on complex topics. By using this approach, I defined my grammar without getting frustrated or making major mistakes. At the same time, overcoming that steep learning curve by doing it myself made me deeply aware of the rules I was implementing.

## 22 Sept- 2026

Finished reading Chapter 2 of the Dragon Book. By using those exact principles and breaking down complex topics with the help of AI, I was able to prompt my way through building a tokenizer. I mainly used AI because, while I knew exactly what I wanted to do at each step, I wasn't familiar with the C++ syntax and special use cases since I am learning a new language. Because the Dragon Book chapters were directly relevant—even though a bit lengthy—I was able to push my way through it. By 10:00 PM, I had a complete, working tokenizer that outputted tokens as per the specification. 

Now for the second phase of this session: I will read Chapter 4 of the Dragon Book on Top-Down Parsers and build my parser by myself. Once I have the complete parser in hand, I will review the code using AI, as I have now gained familiarity with C++ syntax, and Chapter 2 already gave me extensive knowledge on how to build such parsers. It is 3 AM an will meet next day for the execution part .

## 23 Sept -2026

I worked solely on the parser for the next three days, as I had one day off for an exam and another day off due to a full day of work. The parser seems to be a little trickier than I first thought; hence, I was only able to get the relation file node created, and the AST of the relation file was completed by the end of it.

## 27 Sept - 2026

The parser was completed. However, I had to change my relation file AST nodes to a valid structure and table using a vector, as there is no need to print the AST for the relation file. Still, it was a good experience working with trees. There is so much functionality I want to add to this parser to support dynamic user input, but it seems to me now that I am overdoing it and it will not help me finish the whole project. Once I decided on all the functionality as per the specification—and after running some extra tests to my satisfaction by working tirelessly all day—I was able to finish my entire semantic analysis by the end of the day. I was also able to review all changes and functionality. Now, only the execution part is left, which should be done by tomorrow, along with the report. After that, almost everything is set.

## 28 Sept - 2026

The whole execution layer and Data Generator and plus final touches were done today and after rigorous testing started writing the report.

Found a critical bug today when two relations contained the same attribute name, such as `R.b` and `S.b`. The combined schema previously treated both as `b`, so a qualified reference such as `S.b` could incorrectly resolve to the first `b` column. I fixed this by keeping attributes fully qualified in the combined schema (e.g. `R.b`, `S.b`) and making `findColumn()` use a strict qualifier-and-attribute match by introducing attribute struct in execution layer for fine grained control. The join operand evaluation then resolves each attribute against the correct relation, eliminating the ambiguity while preserving normal attribute lookup.

## 29 Sept -2026

- **Main Function:** Refined `main()` to accept command-line input along with a command-line option for `treeonly` mode.
- **Report:** Used the latest statistics produced by the code to complete and polish `REPORT.md`.
- **Testing:** Generated test cases with AI assistance and created a simple test suite covering the required tests. While the application was tested using multiple inputs, only the required tests were included in the suite due to other coursework constraints. The application passes additional tests beyond those in the suite; if time permits, these will be implemented in the future.
- **Submission:** Completed the final video recording and `README.md` file, and submitted the assignment.

## AI Usage:

**My development process followed a modern, AI-assisted workflow driven entirely by my own architectural logic and design.** The application's core logic and structure are firmly rooted in the foundational principles of the **Dragon Book** (*Compilers: Principles, Techniques, and Tools*), which served as the structural bedrock for the entire project.

Because I had mapped out the exact criteria and step-by-step execution path beforehand, I was able to author comprehensive design documents detailing my specific requirements, architectural layers, and edge cases. I utilized AI strictly as an assistant to accelerate the implementation of individual coding files and to flesh out boilerplate for complex edge-case functionality based directly on my specifications.

Once the initial code was generated, I treated it as a draft. I rigorously validated and refined the implementation, performing line-by-line, top-to-bottom code inspections to ensure zero architectural regressions, eliminate bugs, and guarantee that my original logic remained entirely unbroken.