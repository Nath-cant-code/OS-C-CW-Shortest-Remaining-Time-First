#include<stdio.h>

int pointer,pre,a[15],b[15],r[15],count=0,n,small,i,j,m,s=0,k=0,p[10], t[15]={0},w[15]={0},x,y,bs,temp,d=0;

float att=0,awt=0;

int main()

{

printf("enter the number of process\n");

scanf("%d",&n);

for(i=0;i<n;i++)

{

printf("enter the process %d arrival time and burst time\n",i+1);

p[i]=i+1;

scanf("%d%d",&a[i],&b[i]);

count+=b[i];

r[i]=b[i];

}

for(x=0;x<n;x++)

for( y=0;y<n-1;y++)

if(a[y]>a[y+1])

{

temp = a[y+1];

a[y+1] = a[y];

a[y] = temp;

temp=b[y+1];

b[y+1] = b[y];

b[y] = temp;

temp = p[y+1];

p[y+1] = p[y];

p[y] = temp;

}

while(pointer<=count)

{

i=j=m=0 ;

pre=pointer;

k=d+1;

small=9999;

for(i=n-1;i>=0;i--)

{

if(pointer<a[n-1])

{

if(r[i]<=small && r[i]>0 && a[i]<=pointer)

{

small=r[i];

j=i;

}

}

else if(pointer>=a[n-1])

{

if(r[i]<=small && r[i]>0)

{

small=r[i];

j=i;

}

}

}

d=k;

while(a[s+d]==pointer)

d++;

if(pointer==0)

pointer=(a[d]>small)?small:a[d];

else

pointer=(s+d==n-1)?(small<=a[n-2]-pointer)?pointer+small:a[n-2]:(pointer>=a[n-1])? pointer+small :(s+d<n-1)?( (small<=a[s+d]-pointer)?pointer+small:a[s+d] ) :((small<=a[n-1]-pointer)?pointer+small:a[n-1]);

if(pointer-pre>0)

r[j]=r[j]-pointer+pre;

if(pointer-pre>0 && r[j]>=0)

printf(" process %d : executed %d/%d remaining %d\n",p[j],pointer-pre,b[j],r[j]);

if(r[j]==0)

t[j]=pointer-a[j];

}

printf("\n ******OVERALL REVIEW OF SCHEDULING*****\n");

for(i=0;i<n;i++)

{

att+=t[i];

w[i]=t[i]-b[i];

awt+=w[i];

printf("process %d: waiting time:%d turnaround time:%d\n",p[i],w[i],t[i]);

}

printf("\n avarage waiting time :%f\n avarage turnaround time:%f \n \n",awt/n,att/n);

fflush(stdout);        
printf("\nPress Enter to exit...");
getchar();
getchar();

return 0;

}