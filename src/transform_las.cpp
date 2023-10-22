/*
===============================================================================

  FILE:  lasexample.cpp
  
  CONTENTS:
  
    This source code serves as an example how you can easily use LASlib to
    write your own processing tools or how to import from and export to the
    LAS format or - its compressed, but identical twin - the LAZ format.

  PROGRAMMERS:

    info@rapidlasso.de  -  https://rapidlasso.de

  COPYRIGHT:

    (c) 2007-2014, rapidlasso GmbH - fast tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the LICENSE.txt file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    3 January 2011 -- created while too homesick to go to Salzburg with Silke
  
===============================================================================
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

#include "lasreader.hpp"
#include "laswriter.hpp"

void usage(bool wait=false)
{
  fprintf(stderr,"usage:\n");
  fprintf(stderr,"lasexample -t trans.txt in.las out.las\n");
  fprintf(stderr,"lasexample -t trans.txt -i in.las -o out.las -verbose\n");
  fprintf(stderr,"lasexample -t trans.txt -ilas -olas < in.las > out.las\n");
  fprintf(stderr,"lasexample -h\n");
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(1);
}

static void byebye(bool error=false, bool wait=false)
{
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(error);
}

static double taketime()
{
  return (double)(clock())/CLOCKS_PER_SEC;
}

bool load_transform(const std::string &filename, std::vector<std::vector<double>> &total_data) {
  // read four lines in the trans file
  // each line has 4 numbers
  // the last line is 0, 0, 0, 1
  // the first three lines are the transform matrix
  // adapted from Fast-Robust-ICP/io_pc.h
	std::ifstream input(filename);
	std::string line;
	int rows, cols;
	while (getline(input, line)) {
        if(line[0] == 'V' || line[0] == 'M')
            continue;
		std::istringstream iss(line);
		std::vector<double> lineVec;
		while (iss) {
			double item;
			if (iss >> item)
				lineVec.push_back(item);
		}
		cols = lineVec.size();
		total_data.push_back(lineVec);
	}
	if (total_data.size() == 0)
	{
		std::cout << filename << " is empty !! " << std::endl;
		return false;
	}
	rows = total_data.size();
  input.close();
  std::cout << "read trans = " << std::endl;
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++)
      std::cout << total_data[i][j] << " ";
    std::cout << std::endl;
  }
	return true;
}

void transform_point(const std::vector<std::vector<double>> &T, 
        const std::vector<double> &point,
        std::vector<double> &outpoint) {
  // point is a 3d point
  // T is a 4x4 matrix
  // outpoint is a 3d point
  for (int i = 0; i < 3; i++) {
    outpoint[i] = 0;
    for (int j = 0; j < 3; j++) {
      outpoint[i] += T[i][j] * point[j];
    }
    outpoint[i] += T[i][3];
  }
}

int main(int argc, char *argv[])
{
  int i;
  bool verbose = false;
  double start_time = 0.0;
  std::string trans_filename;
  LASreadOpener lasreadopener;
  LASwriteOpener laswriteopener;

  if (argc == 1)
  {
    fprintf(stderr,"%s is better run in the command line\n", argv[0]);
    char file_name[256];
    fprintf(stderr,"enter input file: "); fgets(file_name, 256, stdin);
    file_name[strlen(file_name)-1] = '\0';
    lasreadopener.set_file_name(file_name);
    fprintf(stderr,"enter output file: "); fgets(file_name, 256, stdin);
    file_name[strlen(file_name)-1] = '\0';
    laswriteopener.set_file_name(file_name);
  }
  else
  {
    lasreadopener.parse(argc, argv);
    laswriteopener.parse(argc, argv);
  }

  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] == '\0')
    {
      continue;
    }
    else if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0)
    {
      usage();
    }
    else if (strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"-verbose") == 0)
    {
      verbose = true;
    }
    else if (strcmp(argv[i],"-t") == 0) {
      trans_filename = argv[++i];
    }
    else if (i == argc - 2 && !lasreadopener.active() && !laswriteopener.active())
    {
      lasreadopener.set_file_name(argv[i]);
    }
    else if (i == argc - 1 && !lasreadopener.active() && !laswriteopener.active())
    {
      lasreadopener.set_file_name(argv[i]);
    }
    else if (i == argc - 1 && lasreadopener.active() && !laswriteopener.active())
    {
      laswriteopener.set_file_name(argv[i]);
    }
    else
    {
      fprintf(stderr, "ERROR: cannot understand argument '%s'\n", argv[i]);
      usage();
    }
  }

  if (verbose) start_time = taketime();

  // check input & output
  if (trans_filename.empty()) {
    fprintf(stderr, "ERROR: You should provide transformation.txt like '-t trans.txt'\n");
    usage(argc == 1);
  }
  if (!lasreadopener.active())
  {
    fprintf(stderr,"ERROR: no input specified\n");
    usage(argc == 1);
  }

  if (!laswriteopener.active())
  {
    fprintf(stderr,"ERROR: no output specified\n");
    usage(argc == 1);
  }

  // open lasreader

  LASreader* lasreader = lasreadopener.open();
  if (lasreader == 0)
  {
    fprintf(stderr, "ERROR: could not open lasreader\n");
    byebye(argc==1);
  }

  // open laswriter

  LASwriter* laswriter = laswriteopener.open(&lasreader->header);
  if (laswriter == 0)
  {
    fprintf(stderr, "ERROR: could not open laswriter\n");
    byebye(argc==1);
  }

#ifdef _WIN32
  if (verbose) fprintf(stderr, "reading %I64d points from '%s' and writing them modified to '%s'.\n", lasreader->npoints, lasreadopener.get_file_name(), laswriteopener.get_file_name());
#else
  if (verbose) fprintf(stderr, "reading %lld points from '%s' and writing them modified to '%s'.\n", lasreader->npoints, lasreadopener.get_file_name(), laswriteopener.get_file_name());
#endif

  std::vector<std::vector<double>> T;
  load_transform(trans_filename, T);
  // loop over points and modify them
  int count = 0;
  // where there is a point to read
  while (lasreader->read_point())
  {
    // modify the point
    std::vector<double> point(3, 0);
    point[0] = lasreader->point.get_x();
    point[1] = lasreader->point.get_y();
    point[2] = lasreader->point.get_z();
    std::vector<double> outpoint(3, 0);
    transform_point(T, point, outpoint);
    lasreader->point.set_x(outpoint[0]); // note the changes will not be saved into the file of the reader.
    lasreader->point.set_y(outpoint[1]);
    lasreader->point.set_z(outpoint[2]);

    // write the modified point
    laswriter->write_point(&lasreader->point);
    // add it to the inventory
    laswriter->update_inventory(&lasreader->point);
    if (count < 5) {
      printf("After: %d: X %d Y %d Z %d x %.6f y %.6f z %.6f R %u G %u B %u z scale %.6f z offset %.6f\n", 
          count, lasreader->point.get_X(), lasreader->point.get_Y(), lasreader->point.get_Z(),
          lasreader->point.get_x(), lasreader->point.get_y(), lasreader->point.get_z(), 
          lasreader->point.get_R(), lasreader->point.get_G(), lasreader->point.get_B(), 
          lasreader->point.quantizer->z_scale_factor, lasreader->point.quantizer->z_offset);
    }
    ++count;
  }

  laswriter->update_header(&lasreader->header, TRUE);

  I64 total_bytes = laswriter->close();
  delete laswriter;

#ifdef _WIN32
  if (verbose) fprintf(stderr,"total time: %g sec %I64d bytes for %I64d points\n", taketime()-start_time, total_bytes, lasreader->p_count);
#else
  if (verbose) fprintf(stderr,"total time: %g sec %lld bytes for %lld points\n", taketime()-start_time, total_bytes, lasreader->p_count);
#endif

  lasreader->close();
  delete lasreader;

  return 0;
}
