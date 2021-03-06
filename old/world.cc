#include "world.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// In the constructor of a world the list of people is generated.
World::World() {
	srand(time(NULL));
  num_i = 0;
  num_s = NUM_PEOPLE;
  num_r = 0;
  master_list = generate_people(NUM_PEOPLE);
  infect(0);
}

World::~World() {
  free(master_list);
}

// Returns all people in the world
World::person* World::get_people() {
  return master_list;
}
	
// Creates a list of n people for the world, currently uniformly
// randomly distributed on a (0, 1) torus.
World::person* World::generate_people(int n) {
  person *all_people = (person *) malloc(n * sizeof(person));
	int i;
	for(i = 0; i < n; i++) {
		populate_person(&all_people[i]);
	}
	return all_people; 
}

// Infect the person in master_list at given index and update counters
int World::infect(int index) {
  int first_susceptible = num_r + num_i;
  assert(index >= first_susceptible);
  assert(master_list[first_susceptible].status == SUSCEPTIBLE);

  // Switch places to maintain list order, if necessary.
  if(first_susceptible != index) {
    person temp = master_list[first_susceptible];
    master_list[first_susceptible] = master_list[index];
    master_list[index] = temp;
  }

  master_list[first_susceptible].status = DAYS_INFECTED;
  num_i++;
  num_s--;
  return 0;
}

// Cure the person, but provide the index in i_list
// We will have problems if people start to not recover in
// monotonic increasing order
//
// we are switiching whichever person is about to be cured with
// the last infected person, because the cured person will be susceptible
int World::cure(int index) {
  //sometimes we try to cure already cured people. don't do that!
  if(master_list[index].status == SUSCEPTIBLE) {
    return -1;
  }

  int first_infected = num_r;
  int last_infected = num_r + num_i - 1;
  int first_susceptible = num_r + num_i;
  printf("%d, %d, %d\n", first_infected, index, first_susceptible);
  assert(index >= first_infected && index < first_susceptible);

  assert(master_list[index].status > SUSCEPTIBLE);
  //master_list[index].status = SUSCEPTIBLE;

  // Switch places to maintain list order, if necessary.
  if(last_infected != index) {
    assert(master_list[last_infected].status > SUSCEPTIBLE);
    person temp = master_list[last_infected];
    master_list[last_infected] = master_list[index];
    master_list[index] = temp;
  }


  num_s++;
  num_i--;


  return last_infected;
}

int World::progress_sickness(int index) {
  
  //change to turn into SI model
  return 0;

  master_list[index].status--;
  if(master_list[index].status == SUSCEPTIBLE)
    cure(index);
}


// Prints all people in the world to the terminal
int World::print_people(person* list) {
  int i;
  char buffer[PERSON_BUFFER];

  for(i = 0; i < NUM_PEOPLE; i++) {
    printf("%d\n", i);
		print_person(buffer, &list[i]);
		printf("%s\n", buffer);
	}
	
	return 0;
}
	
// Creates a person with a random location between 0 and 1
int World::populate_person(person * p) {
	p->status = 0;
	p->x = rand()/((float)RAND_MAX + 1);
	p->y = rand()/((float)RAND_MAX + 1);
	return 0;
}

// Takes a person and generates a string representing that person
int World::print_person(char *str, person *p) {
	sprintf(str, "%d, (%f, %f)", p->status, p->x, p->y);
	return 0;
}

// Progress the world one step, propegating infection status when appropriate
int World::step() {
  assert(num_s + num_i + num_r == NUM_PEOPLE);
  int old_num_i = num_i;
  int old_num_r = num_r;
  for(int i = num_r; i < old_num_r + old_num_i; i++) {
    int old_num_s = num_s;
    for(int j = num_r + num_i; j < NUM_PEOPLE; j++) {
      float r = rand()/((float)RAND_MAX + 1);
      if(i == j) {
        delete_me();
        assert(false);
      }
      assert(i != j);
      assert(master_list[j].status == SUSCEPTIBLE && master_list[i].status > SUSCEPTIBLE);
      if(contact_occurs(master_list[i], master_list[j])) {
        infect(j);
      }
    }

    progress_sickness(i);
  }
  
  for(int i = 0; i < NUM_PEOPLE; i++) {
    printf("Person %d, status = %d, x = %f\n", i, master_list[i].status, master_list[i].x);
  }

  //move all people one step to the left
  for(int i = 0; i < NUM_PEOPLE; i++) {
    master_list[i].x = master_list[i].x - DECREMENT_SIZE;
    if(master_list[i].x <= 0) {
      //this person is cured and sent to a random location on the right border
      master_list[i].y = rand()/((float)RAND_MAX + 1);
      master_list[i].x = 1 - (DECREMENT_SIZE * (rand()/((float)RAND_MAX + 1)));
      if(master_list[i].status != SUSCEPTIBLE) {
        master_list[i].status = SUSCEPTIBLE;
      }
    }
  }

  //go through and properly order list. everybody set to susceptible needs to be
  //switched to the end
  for(int i = num_r+num_i-1; i >= 0; i--) {
    if(master_list[i].status == SUSCEPTIBLE) {
      int last_infected = num_r+num_i-1;
      if(i != last_infected) {
        //swap the two
        person temp = master_list[i];
        master_list[i] = master_list[last_infected];
        master_list[last_infected] = temp;
      }
      num_s++;
      num_i--;
    }
  }
  
  for(int i = 0; i < NUM_PEOPLE; i++) {
    printf("Person %d, status = %d, x = %f\n", i, master_list[i].status, master_list[i].x);
  }

  //delete_me();
}

void World::delete_me() {
  printf("TEMPTEMPTEMPTEMPTEMP, s: %d, i: %d, r: %d\n", num_s, num_i, num_r);
  for(int i = 0; i < NUM_PEOPLE; i++) {
    printf("%d ,", master_list[i].status);
  }
  printf("\n");
}


// Randomly simulate contact between people. People are more likely to
// come into contact if they are closer to each other.
bool World::contact_occurs(person a, person b) {
  float d = toroidal_distance(a.x, b.x, a.y, b.y);

  // uniform random variable in (0, 1)
  float u = rand()/((float)RAND_MAX + 1);

  // exponential random variable
  float e = log(1-u)/ (-EXP_PARAMETER);
  return d < e;
}

// Returns distance between two points on a plane, where the edges of the plane
// are assumed to be "wrapped" to the other side.
float World::toroidal_distance(float x1, float x2, float y1, float y2) {
  float dx = x1 - x2;
  float dy = y1 - y2;
  if(dx < 0)
    dx = dx * -1.0;
  if(dy < 0)
    dy = dy * -1.0;

  /* Remove toroidal distance because we are talking about a strip now.
  if(dx > .5)
    dx = 1.0 - dx;
  if(dy > .5)
    dy = 1.0 - dy;*/

  // toroidal distance between people (max = sqrt(2)/2)
  return sqrt(dx*dx + dy*dy);
}
