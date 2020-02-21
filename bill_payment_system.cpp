#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <time.h>
#include <math.h>
#define ATM_NUM 11

using namespace std;

pthread_mutex_t mutex_arr[ATM_NUM];
pthread_cond_t cond_arr[ATM_NUM];
string bill_type[ATM_NUM];
int bill_amount[ATM_NUM];
string output_file;
long int elec;
long int water;
long int gas;
long int cableTV;
long int telec;

vector<vector<string>> datas_of_customers;
int num_threads;
int which_cust[ATM_NUM];
int atm_count[ATM_NUM];
int all_atm_nums[ATM_NUM];
bool is_busy_atm[ATM_NUM];

pthread_mutex_t mut_busy = PTHREAD_MUTEX_INITIALIZER;
//mut_file is used when writing to a file
pthread_mutex_t mut_file = PTHREAD_MUTEX_INITIALIZER;
//below mutexes is used for changing the global bill variables
pthread_mutex_t mut_elec = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_water = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_gas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_cable = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_telec = PTHREAD_MUTEX_INITIALIZER;

//initializes the 10 mutex and 10 condition variables
void initMutexandConds(pthread_mutex_t* mutex_arr, pthread_cond_t* cond_arr){
    for(int i = 1 ;i<ATM_NUM ; i++){
        pthread_mutex_init(&mutex_arr[i],NULL);
        pthread_cond_init(&cond_arr[i],NULL);
    }
}
//used for changing the data of five bill types
//while changing the amount of a bill type it locks its mutex then unlocks it
void payment(int& which_atm){

    if(bill_type[which_atm]=="cableTV"){
        pthread_mutex_lock(&mut_cable);
        cableTV +=bill_amount[which_atm];//add amount to cableTV bill
        pthread_mutex_unlock(&mut_cable);
    }
    else if(bill_type[which_atm]=="electricity"){
        pthread_mutex_lock(&mut_elec);
        elec +=bill_amount[which_atm];//add amount to electricity bill
        pthread_mutex_unlock(&mut_elec);
    }
    else if(bill_type[which_atm]=="telecommunication"){
        pthread_mutex_lock(&mut_telec);
        telec +=bill_amount[which_atm];//add amount to telecommunication bill
        pthread_mutex_unlock(&mut_telec);
    }
    else if(bill_type[which_atm]=="gas"){
        pthread_mutex_lock(&mut_gas);
        gas +=bill_amount[which_atm];//add amount to gas bill
        pthread_mutex_unlock(&mut_gas);
    }
    else{//water
        pthread_mutex_lock(&mut_water);
        water +=bill_amount[which_atm];//add amount to water bill
        pthread_mutex_unlock(&mut_water);
    }
}
//this function is run by customer threads
void *customer_func(void *param) { 
    struct timespec tim;
    int customer = *(int *)param;
    double slp = stoi(datas_of_customers[customer][0]);
    int atm = stoi(datas_of_customers[customer][1]);
    int amount = stoi(datas_of_customers[customer][3]);
    string b_type = datas_of_customers[customer][2];
    tim.tv_sec = 0;
    tim.tv_nsec = slp*1000000;
    nanosleep(&tim, NULL);//sleep the customer for given time

    while(is_busy_atm[atm]){
        //if the atm that the customer wants to use is busy, don't try to take the lock
    }
        pthread_mutex_lock(&mutex_arr[atm]);//take the mutex of the atm
        pthread_mutex_lock(&mut_busy);
        is_busy_atm[atm] = true;//make the atm busy
        pthread_mutex_unlock(&mut_busy);
        which_cust[atm] = customer;//records, which customer is making a transaction
        bill_type[atm] = b_type; //records the operation type going on the atm
        bill_amount[atm] = amount;//recors the operation amount going on the atm

    pthread_cond_signal(&cond_arr[atm]);//send signal to atm
    pthread_mutex_unlock(&mutex_arr[atm]);//release the lock after returning from the atm operation
    pthread_exit(0);
}
//this function is run by atm threads
void *atm_func(void *param) { 
    int which_atm = *(int *)param;//which atm thread is running    
    int size = atm_count[which_atm];
    for(int i = 0 ; i<size ; i++){//don't exit the same thread until everyone who wants to use this atm is complete
        pthread_mutex_lock(&mutex_arr[which_atm]);
     
        while(!is_busy_atm[which_atm]){
            pthread_cond_wait(&cond_arr[which_atm], &mutex_arr[which_atm]);//wait signal from the customer    
        }
    
        payment(which_atm);
        pthread_mutex_lock(&mut_busy);
        is_busy_atm[which_atm] = false;//after operation, make atm not busy again
        pthread_mutex_unlock(&mut_busy);
        pthread_mutex_lock(&mut_file);
        ofstream outfile(output_file, ofstream::app);//append the log to output file
        outfile<<"Customer"<<which_cust[which_atm]+1<<","<<datas_of_customers[which_cust[which_atm]][3]<<"TL,"<<datas_of_customers[which_cust[which_atm]][2]<<endl;
        outfile.close();
        pthread_mutex_unlock(&mut_file);      
        pthread_mutex_unlock(&mutex_arr[which_atm]);
                
    }    
    pthread_exit(0);    
        
}
int main(int argc, char* argv[]) {
    
    string input_file = argv[1];
    string input_without_txt = input_file.substr(0,input_file.length()-4);
    output_file = input_without_txt + "_log.txt";
    ifstream myfile(input_file);
    remove(output_file.c_str());
    ofstream outfile(output_file, ofstream::app);
    

    string line;        
    if(myfile.is_open()){
        getline(myfile, line);         
        num_threads = stoi(line.c_str());
    }
    initMutexandConds(mutex_arr, cond_arr);//initialize mutexes
    pthread_t tid_atms[ATM_NUM];
    pthread_t tid_customers[num_threads];
    
    int all_cust_nums[num_threads];
    //all_atm_nums and all_cust_nums are used for not losing the index of the threads
    for(int i = 1 ; i<ATM_NUM ; i++){
        all_atm_nums[i] = i;
    }
    for(int i = 0 ; i<num_threads ; i++){
        all_cust_nums[i] = i;
    }
    //read the input file line by line
    for(int j = 0 ; j<num_threads ; j++){
        getline(myfile,line);
        stringstream ss(line);
        string token;
         
        vector<string> temp;
        for(int i = 0; i<4 ; i++){
            getline(ss,token,',');
            
            temp.push_back(token);
            if(i==1){
                atm_count[stoi(token)]++;
            }
        }
        datas_of_customers.push_back(temp);//holds all of the data read from the file
        
    }


    myfile.close();
    //create atm threads
    for(int i = 1 ; i<ATM_NUM ; i++){
        pthread_create(&tid_atms[i], NULL, atm_func, &all_atm_nums[i]);
    }
    //create customer threads
    for(int i = 0 ; i<num_threads ; i++){
        pthread_create(&tid_customers[i], NULL, customer_func, &all_cust_nums[i]);
        
    }
    //join all customer threads
    for(int i = 0 ; i<num_threads ; i++){
        pthread_join(tid_customers[i], NULL);
    }
    
    //join all atm threads
    for(int i = 1 ; i<ATM_NUM ; i++){
        pthread_join(tid_atms[i], NULL);
    }
    //log the total amount to the output file
    outfile<<"All payments are completed."<<endl;
    outfile<<"CableTV: "<<cableTV<<endl;
    outfile<<"Electricity: "<<elec<<endl;
    outfile<<"Gas: "<<gas<<endl;
    outfile<<"Telecommunication: "<<telec<<endl;
    outfile<<"Water: "<<water;
    
    outfile.close();
    pthread_exit(0);
}
