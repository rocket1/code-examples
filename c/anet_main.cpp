#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sstream>
#include <string>
#include <fstream>

#include <boost/program_options.hpp>

#include "cfg_class.h"
#include "Serial.H"
#include "Netctrl.H"
#include "Autonetworker.h"
#include "AnetException.h"
#include "LotSummary.h"
#include "TSVDump.h"
//#include "XMLDump.h"
#include "Alert.H"

bool rand_fudge = false;
std::ofstream time_data_out;
static const std::string kOutFile("time_data.txt");

void
usage()
{
    std::cerr << "anet - A utility for debugging MVP Autonetworker\n"
    << "       and report generation (tsv, xml).\n\n"
    << "usage: anet [option]\n\n"
    << "-i Show connection info.\n"
    << "-a Erase ALL the data in the Autonetworker database.\n"
    << "   This is used during testing when you don't care about the data.\n"
    << "   USE WITH CAUTION as it does not prompt you.\n\n"
    << "-f Apply a random fudge to all result values when inserting into the\n"
    << "   AutoNetworker database.  This is provided so if you want to insert\n"
    << "   the same inspection multiple times, you can get non-zero standard deviations.\n\n"
    << "-s Save the current inspection to the Autonetworker.\n\n"
    << "-n Save the current inspection n times (when you just want some data).\n\n"
    << "-l Perform the LotSummary function based on current inspection.\n\n"
    << "-x Write an XML file based on current inspection.\n\n"
    << "-e Erase all inspection EXCEPT for a number to retain.\n"
    << "   E.g. %> anet -e 100 \n"
    << "   will erase all inspections but the most recent 100.\n\n"
    << "-t Write a tsv file based on current inspection.\n\n"
    << "-p Set the tsv generation mode to 'append'.\n\n"
    << "-c Dump time data about Autonetworker saves to a file (time_data.txt).\n\n"
    << "-r [filename] Save repair data specified in an error file.\n\n";


    exit(EXIT_SUCCESS);
}

void
timer_start(struct timeval *start_time)
{
    gettimeofday(start_time, NULL);
}

long
timer_stop(struct timeval *start_time)
{
    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    long secs = end_time.tv_sec - start_time->tv_sec; // not reported 
    long usec = end_time.tv_usec - start_time->tv_usec;

    if (usec < 0) {
        secs--;
        usec += 1000000;
    }

    return usec;
}

void msg(const std::string& msgstr)
{
    std::cerr << msgstr << std::endl;
}

bool
time_data_to_file( long val )
{
    if (!time_data_out.is_open()) {
        struct stat sbuf;
        std::ios_base::openmode mode = stat(kOutFile.c_str(), &sbuf) == 0 ? std::ios::app : std::ios::out;
        time_data_out.open( kOutFile.c_str(), mode );
    }

    if (time_data_out.fail()) {
        return false;
    }

    time_data_out << val << std::endl;
    return true;
}

bool
save_insp( bool dump_time_data, bool do_fake, std::stringstream& out )
{
    try {
   
      out << "SAVE";
      struct timeval time_begin;
      timer_start(&time_begin);
      
      Autonetworker an;
      an.rand_fudge(rand_fudge);
      an.centi_timestamp(intel_timestamp());
      
      if ( !an.save_inspection() ) {
	msg( "Failed saving inspection!" );
	return false;
      }
      
      long time_taken = timer_stop(&time_begin);
      
      if (dump_time_data) {
	time_data_to_file(time_taken);
      }

      out << "\t" << time_taken << "\t" << an.db_size();
      return true;
    }
    catch( std::exception& e ) {
      std::cerr << e.what() << std::endl;
      return false;
    }
}

bool
repair( const std::string& fname )
{
    Autonetworker an;
    an.repair_file( fname );
    an.save_repair();
    return true;
}

bool
info()
{
    const std::string host(gCfg->an_hostname());
    const std::string user(gCfg->an_username());
    const std::string db(gCfg->an_databasename());
    const char* pass = gCfg->an_password();
    u_int npass = pass[0] != '\0' ? strlen(pass) : 0;
    const u_int max_records = gCfg->an_max_inspection_records();
    
    // Obscure password.
    std::string pass_stars;
    while (npass--) {
        pass_stars += "*";
    }

    const std::string div("---------------------------------------\n");
    std::stringstream msg;

    msg << std::endl << "Autonetworker Database Info" << std::endl << div
        << "type: " << Autonetworker::db_enum_to_str(gCfg->an_database()) << std::endl
        << "host: " << host << std::endl
        << "user: " << user << std::endl
        << "pass: " << pass_stars << std::endl
        << "db:   " << db << std::endl << std::endl
        << "max insp records: " << max_records << std::endl;

    std::cerr << msg.str() << std::endl;
    return true;
}

bool
erase( int retain )
{
    std::stringstream m;
    m << "Erasing all but " << retain << " Inspections.";
    msg(m.str());
    Autonetworker an;
    an.erase(retain);
    return true;
}

bool
lotsum()
{
    // Generate means and standard deviations.
    LotSummary lotsum;
    lotsum.init();
    lotsum.summarize();
    return true;
}

bool
xml()
{
    /* XMLDump xmld;
xmld.lane(lane_type_t::FrontLane());
xmld.write(); */

    return true;
}

bool
tsv( bool do_append )
{
    // Dump a file of results to $PCB/error/recipe/lot/xml/tsvdump.tsv
    TSVDump td;
    td.append(do_append);
    td.lane(lane_type_t::FrontLane());
    td.write();
    return true;
}

bool
erase_all()
{
    do {

        std::string reply;
        std::cout << "Do you REALLY want to erase ALL the data in \"" << gCfg->an_databasename() << "\"? (y/n) ";
        std::cin >> reply;

        if (reply == "y" || reply == "Y") {

            const std::string host(gCfg->an_hostname());
            const std::string user(gCfg->an_username());
            const std::string pass(gCfg->an_password());
            const std::string db(gCfg->an_databasename());

            if (host.empty()) {
                msg( "Hostname was empty in Autonetworker::save_inspection()" );
                return false;
            }
            if (user.empty()) {
                msg( "Username was empty in Autonetworker::save_inspection()" );
                return false;
            }
            if (pass.empty()) {
                msg( "Password was empty in Autonetworker::save_inspection()" );
                return false;
            }
            if (db.empty()) {
                msg( "Relational database name was empty in Autonetworker::save_inspection()" );
                return false;
            }

            Autonetworker an;

            msg("Erasing all data...");

            if ( !an.erase_all() ) {
                msg("Failed erasing data.");
                return false;
            }

            return true;
        }
        else if (reply == "n" || reply == "N" ) {
            return true;
        }
        else {
            std::cerr << "Bad response.\n";
        }
    } while(1);
}

int main( int argc, char** argv )
{
    int c;

    bool do_erase_all = false;
    bool do_save_insp = false;
    bool do_lotsum = false;
    bool do_tsv = false;
    bool do_xml = false;
    bool do_erase = false;
    bool do_append = false;
    bool do_fake = false;
    bool dump_time_data = false;
    bool do_repair = false;
    bool do_info = false;

    std::string repair_file;
    int retain = 0;
    int nsaves = 1;

    opterr = 0;

    if (argc < 2) {
        // std::cerr << "Not enough arguments." << std::endl;
        usage();
    }

    while (( c = getopt(argc, argv, "asr:n:lte:pgcxi") ) != -1 ) {
        
        char optoptchar = static_cast<char>(optopt);
        
        switch (c) {

        case 'i':
            do_info = true;
            break;

        case 'a':
            do_erase_all = true;
            break;

        case 'f':
            rand_fudge = true;
            break;

        case 's':
            do_save_insp = true;
            break;

        case 'n':
            nsaves = atoi(optarg);
            break;

        case 'l':
            do_lotsum = true;
            break;

        case 't':
            do_tsv = true;
            break;

        case 'x':
            do_xml = true;
            break;

        case 'e':
            do_erase = true;
            retain = atoi(optarg);
            break;

        case 'p':
            do_append = true;
            break;

        case 'c':
            dump_time_data = true;
            break;

        case 'r':
            do_repair = true;
            repair_file = std::string(optarg);
            break;

        case '?':
            if (optopt == 'e' || optopt == 'n') {
                std::cerr << "Option -" << optoptchar << " requires an argument.";
            }
            else if (isprint (optopt))
            std::cerr << "Unknown option `-" << optoptchar << "'.";
            else
            std::cerr << "Unknown option character `\\x" << optoptchar << "'.";
            usage();

        default:
            std::cerr << "Bad argument parse." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    if (do_info) {
        info();
        exit(EXIT_SUCCESS);
    }

    if (do_repair) {
        repair(repair_file);
        exit(EXIT_SUCCESS);
    }

    if (do_erase) {
        erase(retain);
        exit(EXIT_SUCCESS);
    }

    if (do_erase_all) {
        erase_all();
    }
    else {

        if (do_save_insp) {

            for (int i = 0; i < nsaves; i++ ) {
		std::stringstream out;
                out << (i+1) << "\t";
                save_insp( dump_time_data, do_fake, out );
		std::cout << out.str() << std::endl;
		std::cerr << out.str() << std::endl;
            }
        }

        if (do_lotsum) {
            lotsum();
        }
        
        if (do_tsv) {
            tsv(do_append);
        }

        if (do_xml) {
            xml();
        }
    }

    return 0;
}
