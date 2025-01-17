/* Implementation of PSO using OpenMP.
 *
 * Author: Naga Kandasamy
 * Date: May 2, 2020
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <omp.h>
#include "pso.h"

int pso_omp_solve(char *function, swarm_t * swarm, float xmax, float xmin, int max_iter, int num_threads) {
    int i, j;
    int g = -1;
    int iter = 0;
    float w = 0.79;
    float c1 = 1.49;
    float c2 = 1.49;
    float curr_fitness;
    float r1, r2;
    particle_t *particle, *gbest;

    omp_set_num_threads(num_threads);

    while (iter < max_iter) {
        #pragma omp parallel for default(none) private(i, j, particle, r1, r2, gbest, curr_fitness) \
            shared(function, swarm, xmax, xmin, w, c1, c2, num_threads, g)
        for (i = 0; i < swarm->num_particles; i++) {
            particle = &swarm->particle[i];
            gbest = &swarm->particle[particle->g];
            for (j = 0; j < particle->dim; j++) {
                r1 = (float)rand()/(float)RAND_MAX;
                r2 = (float)rand()/(float)RAND_MAX;
                /* Update particle velocity */
                particle->v[j] = w*particle->v[j] \
                                + c1*r1*(particle->pbest[j] - particle->x[j])\
                                + c2*r2*(gbest->x[j] - particle->x[j]);
                /* Clamp velocity */
                if ((particle->v[j] < -fabsf(xmax - xmin)) || (particle->v[j] > fabsf(xmax - xmin))) 
                    particle->v[j] = uniform(-fabsf(xmax - xmin), fabsf(xmax - xmin));

                /* Update particle position */
                particle->x[j] = particle->x[j] + particle->v[j];
                if (particle->x[j] > xmax)
                    particle->x[j] = xmax;
                if (particle->x[j] < xmin)
                    particle->x[j] = xmin;
            } /* State update */

            /* Evaluate current fitness */
            pso_eval_fitness(function, particle, &curr_fitness);

            /* Update pbest */
            if (curr_fitness < particle->fitness) {
                particle->fitness = curr_fitness;
                for (j = 0; j < particle->dim; j++)
                    particle->pbest[j] = particle->x[j];
            }
        } /* Particle loop */

        /* Identify best performing particle */
        g = pso_omp_best_fitness(swarm, num_threads);
        #pragma omp parallel for default(none) private(i, particle) shared(swarm, g)
        for (i = 0; i < swarm->num_particles; i++) {
            particle = &swarm->particle[i];
            particle->g = g;
        }

#ifdef SIMPLE_DEBUG
        /* Print best performing particle */
        fprintf(stderr, "\nIteration %d:\n", iter);
        pso_print_particle(&swarm->particle[g]);
#endif        

        iter++;
    } /* End of iteration */
    return g;
}

int optimize_using_omp(char *function, int dim, int swarm_size, 
                       float xmin, float xmax, int num_iter, int num_threads)
{
    /* Initialize PSO */
    swarm_t *swarm;
    srand(time(NULL));
    swarm = pso_omp_init(function, dim, swarm_size, xmin, xmax, num_threads);
    if (swarm == NULL) {
        fprintf(stderr, "Unable to initialize PSO\n");
        exit(EXIT_FAILURE);
    }

#ifdef VERBOSE_DEBUG
    pso_print_swarm(swarm);
#endif

    /* Solve PSO */
    int g;
    g = pso_omp_solve(function, swarm, xmax, xmin, num_iter, num_threads);
    if (g >= 0) {
        fprintf(stderr, "Solution:\n");
        pso_print_particle(&swarm->particle[g]);
    }

    pso_free(swarm);
    return g;
}
