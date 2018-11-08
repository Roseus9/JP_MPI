#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>
#include <stdio.h>


int main() {

	//reads the cloud key from file
	FILE* secret_key = fopen("secret.key","rb");
    TFheGateBootstrappingSecretKeySet* key = new_tfheGateBootstrappingSecretKeySet_fromFile(secret_key);
    fclose(secret_key);

    //if necessary, the params are inside the key
    const TFheGateBootstrappingParameterSet* params = key->params;

    //read the 16 ciphertexts of the result
    LweSample* answer = new_gate_bootstrapping_ciphertext_array(16, params);
    //LweSample* answer0 = new_gate_bootstrapping_ciphertext_array(16, params);
    //LweSample* answer1 = new_gate_bootstrapping_ciphertext_array(16, params);
    //LweSample* answer2 = new_gate_bootstrapping_ciphertext_array(16, params);
    //LweSample* answer3 = new_gate_bootstrapping_ciphertext_array(16, params);

    FILE* answer_data = fopen("answer.data","rb");
    for (int i=0; i<16; i++)
        import_gate_bootstrapping_ciphertext_fromFile(answer_data, &answer[i], params);
    fclose(answer_data);

    int16_t int_answer = 0;
    for (int i = 0; i<16; i++) {
        int ai = bootsSymDecrypt(&answer[i], key)>0;
	int_answer |= (ai<<i);
    }

    printf("And the sum is: %d\n", int_answer);
    printf("I hope you remembered what calculation you performed!\n");

    //clean up all pointers
    delete_gate_bootstrapping_ciphertext_array(16, answer);
    delete_gate_bootstrapping_secret_keyset(key);
}
