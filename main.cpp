#include <iostream>
#include <QCoreApplication>
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QDirIterator>
#include <QList>
#include <QProcess>

#define VALGRIND_TEMP_LOG "/valgrind_temp_log"
// ------------------------------------------------------- GET LINE FROM FILE
// this function DOES NOT returns the string with the line feed it ends with
int get_line_from_file(QString *line_str, QFile *file)
{
    *line_str="";
    QDataStream read_stream(file);
    char ch=0;
    int pos=0;
    bool first=true;

    if(file->isOpen() && file->isReadable())
    {
        while(!file->atEnd())
        {
            if(first)
            {
                pos=file->pos();
                first=false;
            }
            read_stream.readRawData(&ch,1);
            if(ch==10)
            {
                return pos;
            }else{
                line_str->append(ch);
            }
        }

    }

    return pos;
}
// ------------------------------------------------------- WRITE LINE TO FILE
void write_line_to_file(QString line_str, QFile *file, bool lf_at_end)
{
    QDataStream write_stream(file);
    uchar uch;
    char ch=0;
    long line_len=line_str.length();
    long c=0;

    if(file->isOpen() && file->isWritable())
    {

        while(c<line_len)
        {
            uch = line_str.at(c).unicode();
            ch=uch;
            write_stream.writeRawData(&ch,1);
            c++;
        }
        if(lf_at_end)
        {
            ch=10;
            write_stream.writeRawData(&ch,1);
        }
    }

}

bool is_valid_ch(char ch, bool first)
{
    if(ch>64 && ch<91){return true;}
    if(ch==95){return true;}
    if(ch>96 && ch<123){return true;}
    if(!first && ch>47 && ch<58){return true;}

    return false;
}

//return amount of function names parsed.
//file must be open already
int parse_func_name_from_cpp(QFile *file,QStringList *list)
{
    int ret=0;
    QDataStream code_stream(file);
    //last visible character
    char last_vis_ch=0;
    char prev_ch=0;
    char ch=0;

    bool in_top_nest=true;
    int nest_level=0;

    //preprocessor directive
    bool in_preproc=false;

    bool in_single_quotes=false;
    bool in_double_quotes=false;

    bool slash=false;
    bool in_line_comment=false;
    bool in_block_comment=false;

    //paren is for parenthesis
    //open parenthesis
    int paren_o_n=0;
    //close parenthesis
    int paren_c_n=0;

    int line=0;
    QString func_str;
    std::string aux_str;

    QString func_name("");

    if(file->isOpen() && file->isReadable())
    {
    while(!file->atEnd())
    {
        prev_ch=ch;
        if(prev_ch>32){last_vis_ch=prev_ch;}
        code_stream.readRawData(&ch,1);
        if(ch==10)
        {
            line++;
        }
        // ----------------------- PARSING

        if(in_preproc)
        {
            if(ch==10 && last_vis_ch!='\\')
            {
                in_preproc=false;
            }
        }else{

            if(in_line_comment)
            {
                // -------------- LINE COMMENT
                if(ch==10)
                {
                    in_line_comment=false;
                }

            }else if(in_block_comment)
            {
                // -------------- BLOCK COMMENT
                if(prev_ch=='*' && ch=='/')
                {
                    in_block_comment=false;
                }

            }else if(in_single_quotes)
            {
                // -------------- SINGLE QUOTES
                if(prev_ch!='\\' && ch=='\'')
                {
                    in_single_quotes=false;
                }

            }else if(in_double_quotes)
            {
                // -------------- DOUBLE QUOTES
                if(prev_ch!='\\' && ch=='"')
                {
                    in_double_quotes=false;
                }

            }else{
                // -------------- REST
                if(ch=='#'){in_preproc=true;}else
                if(ch=='/')
                {
                    if(prev_ch=='/')
                    {
                        in_line_comment=true;
                    }
                }else
                if(prev_ch=='/' && ch=='*'){in_block_comment=true;}else
                if(ch=='\''){in_single_quotes=true;}else
                if(ch=='"'){in_double_quotes=true;}else{
                    if(!in_top_nest)
                    {

                        if(ch=='}')
                        {
                            if(nest_level>0){nest_level--;}
                            if(nest_level==0)
                            {
                                func_str.append(ch);
                                in_top_nest=true;
                            }
                        }
                        if(ch=='{')
                        {

                            nest_level++;
                        }
                    }else{
                        func_str.append(ch);
                        if(ch=='{')
                        {

                            nest_level++;
                            in_top_nest=false;
                        }



                    }//else in top nest
                }//else not comment or quotes
            }//else in rest

        }//not in_preproc
    }
    }

    func_str=func_str.trimmed();
    func_str=func_str.simplified();
    func_name=func_str.section("{}",ret,ret);
    std::string str;
    while(!func_name.isEmpty())
    {

        func_name=func_name.section("(",0,0);

        func_name=func_name.trimmed();
        func_name=func_name.section(" ",-1,-1);
        if(func_name.contains("::"))
        {
            func_name=func_name.section("::",1,-1);
        }
        if(func_name.contains(":"))
        {
            func_name=func_name.section(":",0,0);
        }
        func_name=func_name.trimmed();
        list->append(func_name);


        QString aux=func_name;
        aux.prepend(":::");
        aux.prepend(file->fileName());

        str=aux.toStdString();
        std::cout << str << "\n";


        if(!func_name.isEmpty())
        {
            ret++;
            func_name=func_str.section("{}",ret,ret);
        }
    }
    func_str="";

    return ret;
}

int parse_errors_from_log(QFile *file,QStringList *list)
{
    int ret=0;
    char ch=0;
    char prev_ch=0;
    char ant_ch=0;
    QDataStream code_stream(file);
    QString error_str;
    std::cout << "\n\nGETTING ERROR FUNCTION\n\n";
    if(file->isOpen() && file->isReadable())
    {
        std::cout << "\n\nGETTING ERROR\n\n";
        while(!file->atEnd())
        {
            ant_ch=prev_ch;
            prev_ch=ch;
            code_stream.readRawData(&ch,1);

            if(ant_ch==10 && prev_ch=='}' && ch==10)
            {
                std::cout << "\n\nGOT ERROR>>" << error_str.length() << "\n\n";
                if(error_str.length()>3)
                {
                    list->append(error_str);
                    ret++;
                }
                error_str="";
            }else{
                error_str.append(ch);
            }
        }
    }

    return ret;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    bool proceed=true;
    // ------------------------------------- VERIFY ARGUMENTS
    int arg_n=argc;
    if(arg_n!=4)
    {
        std::cout << "\n\n Correct usage:\n\n";
        std::cout << "parse_valgrind_suppressions [Executable] [Directory for the code] [Output file]";
        std::cout << "\n\n";
        return 1;
    }
    std::cout << "\nRunning Valgrind...\n";
    // ------------------------------------- VARIABLES
    QString exe_file_name(argv[1]);
    QProcess valgrind_process;
    QStringList valgrind_args;

    QString temp_log_file_name(argv[0]);
    temp_log_file_name=temp_log_file_name.section("/",0,-2);
    temp_log_file_name.append(VALGRIND_TEMP_LOG);

    QString valgrind_log_file_arg("--log-file=");
    valgrind_log_file_arg.append(temp_log_file_name);

    valgrind_args.insert(0,"--leak-check=full");
    valgrind_args.insert(1,"--gen-suppressions=all");
    valgrind_args.insert(2,valgrind_log_file_arg);
    valgrind_args.insert(3,exe_file_name);
    valgrind_process.setProgram("valgrind");
    valgrind_process.setArguments(valgrind_args);

    // ------------------------------------- VALGRIND PROCESS
    valgrind_process.start();
    valgrind_process.waitForFinished(-1);

    // ------------------------------------- GOING THROUGH CODE DIRECTORY
    int valgrind_exit_code=valgrind_process.exitCode();
    int valgrind_exit_status=valgrind_process.exitStatus();
    //std::cout << "\n\nEXIT STATUS:" << valgrind_exit_status;
    //std::cout << "\n\nEXIT CODE:" << valgrind_exit_code;
    QFile temp_file(temp_log_file_name);
    if(temp_file.exists() && valgrind_exit_status==QProcess::NormalExit && valgrind_exit_code==0)
    {
        QStringList func_list;
        int func_list_len=0;

        QStringList error_list;
        int error_list_len=0;

        QString real_errors_log=" REAL ERRORS FOUND.";
        int real_errors_n=0;

        QString output_file_name(argv[3]);
        QFile output_file(output_file_name);

        QString code_dir(argv[2]);

        QFile cpp_file;
        QDirIterator dir_it(code_dir,QDir::Files|QDir::NoDotAndDotDot,QDirIterator::Subdirectories|QDirIterator::FollowSymlinks);
        QString dir_next_file_name;
        std::string aux_str;
        int code_files_n=0;

        QFile log_file(temp_log_file_name);

        //in_first_ch doesn't count in comment characters
        std::cout << "\nGoing through code...\n";
        while(dir_it.hasNext())
        {
            dir_next_file_name=dir_it.next();
            cpp_file.setFileName(dir_next_file_name);
            if(cpp_file.exists())
            {
                QString ext=cpp_file.fileName().section(".",-1,-1);
                if(ext=="cpp" || ext=="c")
                {
                    // --------------- PARSING CODE
                    if(cpp_file.open(QIODevice::ReadOnly))
                    {
                        func_list_len+=parse_func_name_from_cpp(&cpp_file,&func_list);
                        cpp_file.close();
                        code_files_n++;
                    }else{
                        aux_str=dir_next_file_name.toStdString();
                        std::cout << "\nError opening " << aux_str << ".\n";
                    }

                }//is cpp file
            }//file exists

        }
        if(code_files_n==0)
        {
            std::cout << "\nNo code files parsed.\n";
            return 2;
        }else{
            aux_str=QString::number(code_files_n).toStdString();
            std::cout << "\n" << aux_str << " code files parsed." << ".\n";
        }
        std::cout << "\n" << func_list_len << " functions parsed.";
        std::cout << "\nParsing leaks from valgrind log file...\n";
        // ------------------------------------- PARSING VALGRIND LOG
        if(log_file.open(QIODevice::ReadOnly))
        {
            error_list_len=parse_errors_from_log(&log_file,&error_list);
            log_file.close();
        }else{
            std::cout << "\nError opening valgrind log file for parsing...\n";
            return 3;
        }
        std::cout << "\n" << error_list_len << " errors parsed.\n";
        std::cout << "\nGenerating output...\n";
        if(output_file.open(QIODevice::WriteOnly))
        {
            if(func_list_len>0 && error_list_len>0)
            {
                int fc=0;
                int ec=0;
                while(ec<error_list_len)
                {
                    while(fc<func_list_len)
                    {
                        QString cur_error=error_list.at(ec);
                        QString cur_func=func_list.at(fc);

                        QString fun_cur_func=cur_func;
                        fun_cur_func.prepend("fun:");

                        QString spc_cur_func=cur_func;
                        spc_cur_func.prepend(": ");

                        if(cur_error.contains(fun_cur_func) || cur_error.contains(spc_cur_func))
                        {
                            QString aux_qstr="\nFUNC IN ERROR:>:>:";
                            aux_qstr.append(func_list.at(fc));
                            aux_qstr.append(" EC: ");
                            aux_qstr.append(QString::number(ec));
                            aux_qstr.append(" FC: ");
                            aux_qstr.append(QString::number(fc));
                            aux_qstr.append(" : ");
                            aux_qstr.append(QString::number(real_errors_n));
                            aux_str=aux_qstr.toStdString();
                            std::cout << aux_str;

                            QString *bla=new QString("bla");

                            aux_qstr="\n\nERROR>>>>>>>";
                            aux_qstr.append(error_list.at(ec));
                            aux_qstr.append("<<<<<<<<END ERROR\n\n");
                            aux_qstr.append(bla);
                            aux_str=aux_qstr.toStdString();
                            //std::cout << aux_str;

                            write_line_to_file(error_list.at(ec),&output_file,true);
                            real_errors_n++;
                            break;
                        }

                        fc++;
                    }
                    fc=0;
                    ec++;
                }

                real_errors_log.prepend(QString::number(real_errors_n));
                real_errors_log.prepend("\n");
                write_line_to_file(real_errors_log,&output_file,true);

                aux_str=real_errors_log.toStdString();
                std::cout << aux_str;
            }


            output_file.close();
        }else{
            std::cout << "\n Failed to create output file.\n";
            return 4;
        }
    }else{
        // ------------------------------- ERROR ON VALGRIND
        QString err_str=valgrind_process.readAllStandardError();
        std::string str=valgrind_log_file_arg.toStdString();
        std::cout << "\n\nERROR: " <<  str << "\n\n" << argv[0] << "\n\n";
        str=err_str.toStdString();
        std::cout << "VALGRIND ERROR: " << str << "\n\n";
    }

    return 0;
}

